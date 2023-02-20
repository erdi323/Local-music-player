#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "utils.h"
#include <QSqlDatabase>
#include "song.h"
//数据库管理类
class DatabaseManager
{
private:
    DatabaseManager();
    DatabaseManager(const DatabaseManager&);
    ~DatabaseManager();
    QString m_databaseType; //数据库类型
    QString m_databaseName; //数据库文件名
    QSqlDatabase m_sqlDatabase; //Qt数据库连接对象

    bool initSong();//初始歌曲列表

public:
    static DatabaseManager& getInstance();
    void destroy();//数据库定义释放接口
    bool init();//初始化接口
    bool addSong(const Song& song);//添加歌曲
    bool clearAllSong();//清空歌曲
    bool querySongs(QVector<Song*>& songsResult); //恢复上次播放列表
};

#endif // DATABASEMANAGER_H
