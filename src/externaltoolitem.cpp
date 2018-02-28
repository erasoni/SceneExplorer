//SceneExplorer
//Exploring video files by viewer thumbnails
//
//Copyright (C) 2018  Ambiesoft
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "consts.h"
#include "settings.h"

#include "externaltoolitem.h"

ExternalToolItem ExternalToolItem::Load(int i, Settings& settings)
{
    QString group = Consts::KEY_EXTERNALTOOLS_GROUPPRIX + QString::number(i);

    settings.beginGroup(group);
    QString name = settings.valueString(Consts::KEY_EXTERNALTOOLS_NAME);
    QString exe = settings.valueString(Consts::KEY_EXTERNALTOOLS_EXE);
    QString arg = settings.valueString(Consts::KEY_EXTERNALTOOLS_ARG);
    settings.endGroup();

    return ExternalToolItem(name,exe,arg);
}
void ExternalToolItem::Save(int i, Settings& settings)
{
    QString group = Consts::KEY_EXTERNALTOOLS_GROUPPRIX + QString::number(i);

    settings.beginGroup(group);
    settings.setValue(Consts::KEY_EXTERNALTOOLS_NAME, name_);
    settings.setValue(Consts::KEY_EXTERNALTOOLS_EXE, exe_);
    settings.setValue(Consts::KEY_EXTERNALTOOLS_ARG, arg_);
    settings.endGroup();

}
