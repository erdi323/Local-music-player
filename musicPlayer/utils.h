#ifndef UTILS_H
#define UTILS_H
//通用方法定义头文件


#include <QDebug>
/*
文件名 __FILE__
行号 __LINE__
函数名 __FUNCTION__
函数名及其参数  __PRETTY_FUNCTION__
*/
static QString __SOURCE(QString file, int line, QString func)//自定义显示函数
{
    QStringList File = file.split('\\');; //一个反斜杠\表示转移字符，要使用反斜杠本身需要用'\\'
    QStringList Func = func.split(' ');
    return File.last() + " 第" + QString::number(line) + "行 " + Func.last();

}
#define log (qDebug() <<"["<< __SOURCE(__FILE__, __LINE__, __PRETTY_FUNCTION__) <<"]")


#endif // UTILS_H
