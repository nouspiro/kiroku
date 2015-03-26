/****************************************************************************
*
*    Kiroku, software to record OpenGL programs
*    Copyright (C) 2014  Dušan Poizl
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

#ifndef SHADERS_H
#define SHADERS_H

const char *rgb2yuv_source_vertex =
        "#version 130\n"
        "in vec4 vertex;\n"
        "out vec2 texCoord;\n"
        "void main(){\n"
        "gl_Position = gl_Vertex;\n"
        "texCoord = gl_Vertex.xy*0.5+0.5;\n"
        "texCoord.y = 1.0-texCoord.y;\n"
        "}";

const char *rgb2yuv_source_frag =
        "#version 130\n"
        "uniform sampler2D tex;\n"
        "uniform vec2 size;\n"
        "uniform mat4 rgb2yuv;\n"
        "in vec2 texCoord;\n"
        "out float y;\n"
        "out float u;\n"
        "out float v;\n"
        "void main(){\n"
        "vec4 yuv = rgb2yuv*vec4(texture(tex, texCoord).rgb, 1.0);\n"
        "y = yuv.x;\n"
        "u = yuv.y;\n"
        "v = yuv.z;\n"
        "}";

#endif // SHADERS_H
