/****************************************************************************
*
*    Kiroku, software to record OpenGL programs
*    Copyright (C) 2016  Dušan Poizl
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

#include "injector.h"
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <dlfcn.h>

Injector::Injector()
{
}

int Injector::attach(int _pid)
{
    pid = _pid;
    return ptrace(PTRACE_ATTACH, pid, NULL, NULL);
}

int Injector::detach()
{
    long ret = ptrace(PTRACE_DETACH, pid, 0, 0);
    pid = 0;
    return ret;
}

bool Injector::dlopen(QString path, size_t dlopen)
{
    struct user_regs_struct regs;
    struct user_regs_struct newregs;
    long trace;

    QFileInfo info(path);
    if (!info.exists())
        return false;

    int status;
    waitpid(pid, &status, 0);
    qDebug() << "WIFEXITED" << WIFEXITED(status);
    qDebug() << "WEXITSTATUS" << WEXITSTATUS(status);
    qDebug() << "WIFSIGNALED" << WIFSIGNALED(status);
    qDebug() << "WTERMSIG" << WTERMSIG(status);
    qDebug() << "WIFSTOPPED" << WIFSTOPPED(status);
    qDebug() << "WSTOPSIG" << WSTOPSIG(status);
    qDebug() << "WIFCONTINUED" << WIFCONTINUED(status);
    if(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0)
    {
        qDebug() << "Can't get registers";
        return false;
    }

    newregs = regs;
    newregs.rsp -= 256;//allocate 256 byte on stack
    newregs.rax = 0;
    newregs.rdi = newregs.rsp;
    newregs.rsi = RTLD_LAZY;
    newregs.rdx = dlopen;

    QByteArray p = path.toUtf8();
    if(!pokeData(p.constData(), (void*)newregs.rsp, p.length()))
        return false;

    unsigned char callrdx[4] = {0xff, 0xd2, 0xcc, 0x90};
    char codeBackup[4];
    peekData(codeBackup, (void*)regs.rip, sizeof(codeBackup));
    pokeData(callrdx, (void*)regs.rip, sizeof(callrdx));

    trace = ptrace(PTRACE_SETREGS, pid, 0, &newregs);
    trace = ptrace(PTRACE_CONT, pid, 0, 0);
    waitpid(pid, &status, 0);

    trace = ptrace(PTRACE_GETREGS, pid, 0, &newregs);

    if(!newregs.rax)
        return false;

    //restore original
    if(!pokeData(codeBackup, (void*)regs.rip, sizeof(codeBackup)))
        return false;

    trace = ptrace(PTRACE_SETREGS, pid, NULL, &regs);
    //ptrace(PTRACE_CONT, pid, 0, 0);

    return true;
}

bool Injector::installTrampoline(const void *functionaddr, const void *target)
{
    int status;
    //if(ptrace(PTRACE_INTERRUPT, pid, 0, 0)<0)
        //return false;

    //waitpid(pid, &status, 0);
    //qDebug() << "WIFSTOPPED" << WIFSTOPPED(status);

    unsigned char trampolineCode[] = {0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //movabs $0x0,%rax
                                     0xff, 0xe0};//jmpq *%rax

    memcpy(trampolineCode+2, &target, 8);

    unsigned char original[32];
    peekData(original, functionaddr, sizeof(original));

    pokeData(trampolineCode, functionaddr, sizeof(trampolineCode));
    //    return false;

//    if(ptrace(PTRACE_CONT, pid, 0, 0)<0)
  //      return false;

    return true;
}

bool Injector::findLibrary(QString library, size_t *mappedaddr, QString &path)
{
    QFile fr(QString("/proc/%1/maps").arg(pid));
    if(!fr.open(QIODevice::ReadOnly))return false;
    while(1)
    {
        QString line = QString(fr.readLine());
        if(line.length()==0)break;
        if(line.contains(library) && line.contains("r-xp"))
        {
            QRegExp regex("^[0-9a-f]*");
            QRegExp pathreg("([^ ]*)\n$");
            regex.indexIn(line);
            pathreg.indexIn(line);

            bool ok;
            *mappedaddr = regex.cap().toULongLong(&ok, 16);
            path = pathreg.cap(1);
            if(!ok)
            {
                return false;
            }
            return true;
        }
    }
    return false;
}

bool Injector::findSymbol(QString libpath, QString symbol, size_t *symboladdr)
{
    QProcess objdump;
    QStringList arguments;
    arguments << "-tT" << libpath;
    objdump.start("objdump", arguments);
    objdump.waitForFinished();
    QString out = QString(objdump.readAll());
    QList<QString> lines = out.split('\n');

    QRegExp symbolReg(" "+symbol+"$");
    foreach (const QString &line, lines)
    {
        if(line.contains(symbolReg))
        {
            QRegExp regex("^0*([0-9a-f]*)[^0]*([0-9a-f]*)");
            regex.indexIn(line);
            bool ok;
            *symboladdr = regex.cap(1).toULongLong(&ok, 16);
            return true;
        }
    }
    return false;
}

bool Injector::peekData(void *data, const void *start, size_t len)
{
    long d;
    size_t i=0;
    while(i<len)
    {
        d = ptrace(PTRACE_PEEKDATA, pid, (char*)start+i, NULL);
        //cout << hex << d << dec << endl;
        memcpy((char*)data+i, &d, sizeof(long));
        i += sizeof(long);
    }
    return 0;
}

bool Injector::pokeData(const void *data, const void *start, size_t len)
{
    long d;
    size_t i=0;
    while(i<len)
    {
        memcpy(&d, (char*)data+i, sizeof(long));
        if(-1==ptrace(PTRACE_POKEDATA, pid, (char*)start+i, d))
            return false;

        i += sizeof(long);
    }
    return true;
}



