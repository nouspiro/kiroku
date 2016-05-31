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

#ifndef INJECTOR_H
#define INJECTOR_H

#include <QString>

class Injector
{
    int pid;
public:
    Injector();
    int attach(int _pid);
    int detach();
    bool dlopen(QString path, size_t dlopen);
    bool installTrampoline(const void *functionaddr, const void *target);
    bool findLibrary(QString library, size_t *mappedaddr, QString &path);
    bool findSymbol(QString libpath, QString symbol, size_t *symboladdr);
protected:
    bool peekData(void *data, const void *start, size_t len);
    bool pokeData(const void *data, const void *start, size_t len);
};

#endif // INJECTOR_H
