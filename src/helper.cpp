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

#include <QDir>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QDesktopServices>
#include <QSet>
#include <QCoreApplication>
#include <QLocale>
#include <QComboBox>
#include <QList>
#include <QCommandLineParser>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QSqlError>

#include "../../lsMisc/stdQt/stdQt.h"

#include "commandoption.h"
#include "consts.h"
#include "osd.h"
#include "helper.h"


using namespace Consts;






QString normalizeFile(const QString& file)
{
	return QFileInfo(file).absoluteFilePath();
}
void nomalizeDirAndName(const QString& full, QString&dir, QString& name)
{
    QFileInfo fi(full);
    dir = normalizeDir(fi.absolutePath());
    name = fi.fileName();
}
void Info(QWidget* parent, QString message)
{
    QMessageBox msgBox(parent && parent->isVisible() ? parent:nullptr);
	
    msgBox.setWindowTitle(APPNAME_DISPLAY);
	// msgBox.setInformativeText(message);
	msgBox.setText(message);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.exec();
}
void Alert(QWidget* parent, QString message)
{
    QMessageBox msgBox(parent && parent->isVisible() ? parent:nullptr);
	
	// msgBox.setInformativeText(message);
	msgBox.setText(message);
	msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Warning);
	msgBox.exec();
}
bool YesNo(QWidget* parent,
           QString message,
           QMessageBox::Icon icon)
{
    QMessageBox msgBox(parent && parent->isVisible() ? parent:nullptr);
	
    msgBox.setWindowTitle(APPNAME_DISPLAY);
	msgBox.setText(message);
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(icon);
	return msgBox.exec() == QMessageBox::Yes;
}

static QString getBytecode1(char c)
{
    QString ret;
    switch( (c & 0xF0)>>4 )
    {
    case 0: ret+="0";break;
    case 1: ret+="1";break;
    case 2: ret+="2";break;
    case 3: ret+="3";break;
    case 4: ret+="4";break;
    case 5: ret+="5";break;
    case 6: ret+="6";break;
    case 7: ret+="7";break;
    case 8: ret+="8";break;
    case 9: ret+="9";break;
    case 0xa: ret+="a";break;
    case 0xb: ret+="b";break;
    case 0xc: ret+="c";break;
    case 0xd: ret+="d";break;
    case 0xe: ret+="e";break;
    case 0xf: ret+="f";break;
default:
        Q_ASSERT(false);
    }
    switch(c & 0x0F)
    {
    case 0: ret+="0";break;
    case 1: ret+="1";break;
    case 2: ret+="2";break;
    case 3: ret+="3";break;
    case 4: ret+="4";break;
    case 5: ret+="5";break;
    case 6: ret+="6";break;
    case 7: ret+="7";break;
    case 8: ret+="8";break;
    case 9: ret+="9";break;
    case 0xa: ret+="a";break;
    case 0xb: ret+="b";break;
    case 0xc: ret+="c";break;
    case 0xd: ret+="d";break;
    case 0xe: ret+="e";break;
    case 0xf: ret+="f";break;
default:
        Q_ASSERT(false);
    }

    Q_ASSERT(ret.size()==2);
    return ret;
}
static QString getBytecode(const char* pByte, int size)
{
    QString ret;
    for(int i=0; i< size; ++i)
    {
        ret += getBytecode1(pByte[i]);
    }
    return ret;
}

QString createSalient(const QString& file, const qint64& size)
{
    QFile f(file);
    if(!f.open(QIODevice::ReadOnly))
        return QString();

    qint64 readed=0;
    char buff[20]={0};
    readed=f.read(buff, 20);
    if(readed <= 0)
        return QString();

    QString ret = getBytecode(buff, readed);

    if(size <= 20)
        return ret;

    if(!f.seek(size-20))
        return QString();
    readed=f.read(buff, 20);
    if(readed != 20)
        return QString();
    ret += "-";
    ret += getBytecode(buff, 20);

    if(size <= 40)
        return ret;

    if(!f.seek(size/3))
        return QString();
    readed=f.read(buff, 20);
    if(readed != 20)
        return QString();
    ret += "-";
    ret += getBytecode(buff, 20);

    if(!f.seek( size/3*2))
        return QString();
    readed=f.read(buff, 20);
    ret += "-";
    ret += getBytecode(buff, 20);

    return ret;
}



QString getUUIDFromThumbfile(const QString& file)
{
    int i = file.lastIndexOf('.');
    if(i < 0)
        return QString();
    QString ret = file.left(i);
    if(ret.length() < 2)
        return QString();
    ret = ret.left(ret.length()-2);
    return ret;
}


Qt::WindowFlags GetDefaultDialogFlags()
{
    return ((Qt::WindowTitleHint | Qt::WindowCloseButtonHint| Qt::WindowFlags()) & ~Qt::WindowContextHelpButtonHint);
}


QString GetSortColumnName(SORTCOLUMNMY sc)
{
	switch (sc)
	{
	case SORT_NONE:return QString();
	case SORT_FILENAME:return QObject::tr("Filename");
    case SORT_FULLNAME:return QObject::tr("Fullname");
	case SORT_SIZE:return QObject::tr("Size");
	case SORT_WTIME:return QObject::tr("Wtime");
	case SORT_RESOLUTION: return QObject::tr("Resolution");
	case SORT_DURATION:return QObject::tr("Duration");
	case SORT_BITRATE:return QObject::tr("Bitrate");
	case SORT_OPENCOUNT:return QObject::tr("Open count");
    case SORT_LASTACCESS:return QObject::tr("Last Access");
	default:
		Q_ASSERT(false);

	}
	return QString();
}

QString GetAppDir()
{
    return QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
}

QString GetSystemDefaultLang()
{
    return QLocale::languageToString(QLocale::system().language());
}

void InsertUniqueTextToComboBox(QComboBox& combo, const QString& text)
{
    if(text.isEmpty())
        return;

    QList<int> dels;
    for(int i=0 ; i < combo.count(); ++i)
    {
        QString t = combo.itemText(i);
        if(t==text)
            dels.append(i);
    }

    QList<int>::const_reverse_iterator rit = dels.crbegin();
    for (; rit != dels.crend(); ++rit)
    {
        int deli = *rit;
        combo.removeItem(deli);
    }

    combo.insertItem(0, text);
}

QString dq(const QString& s)
{
    if (s.isEmpty())
        return "\"\"";

    if (s[0] == '\\')
        return s;

    if(s[0] == '"')
        return s;

    if (!s.contains(" ") && !s.contains(","))
        return s;

    return "\"" + s + "\"";
}
QString undq(QString s)
{
    if(s.isEmpty())
        return s;
    if(s[0] != '"')
        return s;
    if(!s.endsWith('"'))
        return s;

    s = s.right(s.length()-1);
    s = s.left(s.length()-1);

    return s;
}

bool processCommandLine(QString* helpText)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("SceneExplorer");

    QCommandLineOption helpOption = parser.addHelpOption();
    QCommandLineOption versionOption = parser.addVersionOption();
//    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
//    parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    // A boolean option with a single name (-p)
//    QCommandLineOption showProgressOption("p", QCoreApplication::translate("main", "Show progress during copy"));
//    parser.addOption(showProgressOption);

    // A boolean option with multiple names (-f, --force)
//    QCommandLineOption forceOption(QStringList() << "f" << "force",
//                                   QCoreApplication::translate("main", "Overwrite existing files."));
//    parser.addOption(forceOption);

    // An option with a value
    QCommandLineOption dbDirectoryOption(QStringList() << "d" << "database-directory",
                                             QCoreApplication::translate("main", "Set database directory <directory>."),
                                             "directory");
    parser.addOption(dbDirectoryOption);



//    QStringList lll = QCoreApplication::arguments();
//    QString sss;
//    for(QString s: lll)
//    {
//        sss += s;
//        sss += "   ";
//    }
//    Alert(nullptr,"arg=" + sss);

    // Process the actual command line arguments given by the user
    parser.parse(QCoreApplication::arguments());

    // const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

    // bool showProgress = parser.isSet(showProgressOption);
    // bool force = parser.isSet(forceOption);

    if(helpText != nullptr)
    {
        *helpText = parser.helpText();
        return false;
    }
    if(parser.isSet(helpOption))
    {
        // Info(nullptr, parser.sh);
        parser.showHelp();
        return false;
    }
    if(parser.isSet(versionOption))
    {
        parser.showVersion();
        return false;
    }

    QString dbdir = parser.value(dbDirectoryOption);
    QString doc;

    const QStringList args = parser.positionalArguments();
    if(!args.isEmpty())
    {
        doc = parser.value(QString());
        doc = args[0];
        if(doc=="/?")
        {
            parser.showHelp();
            return false;
        }
    }

	Q_ASSERT(!gpCommandOption.get());
	gpCommandOption.reset(new CommandOption(dbdir, doc));

    return true;
}

QString ExpandEnv(const QString& str)
{
    static QRegExp rx("(\\$\\{\\w+\\})");
    QString result;
    int prevpos = 0;
    int pos = 0;

    while ((pos = rx.indexIn(str, pos)) != -1)
    {
        result += str.mid(prevpos,pos-prevpos);
        int matchedlen = rx.matchedLength();
        QString s = str.mid(pos,matchedlen);
        if(s==STR_ENV_SCENEEXPLORER_ROOT)
        {
            static QString thisdir = GetAppDir();
            result += thisdir;
        }
        else
        {
            result += str.mid(pos, matchedlen);
        }
        pos += matchedlen;
        prevpos = pos;
    }
    result += str.mid(prevpos);

    return result;
}


// In linux, constructor with sql string cause sigxxx, so this is temporal fix
QSqlQuery myPrepare(const QString& sql)
{
    QSqlQuery q;
    if(!q.prepare(sql))
    {
        qDebug() << q.lastError();
        Q_ASSERT(false);
        return q;
    }
    return q;
}

QString getClipboardText()
{
    return QApplication::clipboard()->text();
}
void setClipboardText(const QString& text)
{
    QApplication::clipboard()->setText(text);
}
bool IsClipboardTagDataAvalable()
{
	QString clipText = getClipboardText();
	QStringList lines = clipText.split('\n');
	if (lines.isEmpty())
		return false;

	if (lines[0].startsWith(STR_TAG_ENTRY_SIGNATURE))
		return true;

	return false;
}

bool isLegalFileExt(QString ext)
{
    if(ext.isEmpty())
        return false;

    if(ext[0]=='.')
    {
        ext = ext.right(ext.length()-1);
        return isLegalFileExt(ext);
    }

    if(ext.contains('/') || ext.contains('\\'))
        return false;

    return isLegalFilePath(ext, nullptr);
}
bool isThumbFileName(const QString& file)
{
    // ex) 58c4d22e-8b8b-4773-9fac-80a69a8fa880-5.jpg
    static QRegExp rx(
                "^"
                "[a-fA-F0-9]{8}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{12}"
                "-"
                "[0-9]+"
                "\\."
                "[a-zA-Z0-9]+");

    Q_ASSERT(rx.isValid());
    return rx.exactMatch(QFileInfo(file).fileName());
}
bool isUUID(const QString& s)
{
    // ex) B4149711-6824-4606-B601-AC89C1BEC092
    static QRegExp rx(
                "^"
                "[a-fA-F0-9]{8}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{4}"
                "-"
                "[a-fA-F0-9]{12}"
                "$");
    Q_ASSERT(rx.isValid());
    return rx.exactMatch(s);
}

std::wstring qToStdWString(const QString &str)
{
#ifdef _MSC_VER
 return std::wstring((const wchar_t*)str.utf16());
#else
 return str.toStdWString();
#endif
}
QString stdWToQString(const std::wstring &str)
{
#ifdef _MSC_VER
 return QString::fromUtf16((const ushort*)str.c_str());
#else
 return QString::fromStdWString(str);
#endif
}

bool IsSubDir(const QString& parent, const QString& child)
{
    QDir childDir(child);
    do
    {
        if(QDir(parent)==childDir)
            return true;
    } while(childDir.cdUp());

    return false;
}
