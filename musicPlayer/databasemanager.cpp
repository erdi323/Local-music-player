#include "databasemanager.h"
#include <QSqlError> //用于输出数据库的错误信息
#include <QSqlQuery>
DatabaseManager::DatabaseManager()
    : m_databaseType("QSQLITE"), m_databaseName("player.db")
{
    m_sqlDatabase = QSqlDatabase::addDatabase(m_databaseType);//传一个参数(数据库驱动类型)-指定数据库类型
    //m_sqlDatabase = QSqlDatabase::addDatabase(m_databaseType,connectionName);传入两参数(数据库驱动类型和连接名称)
    m_sqlDatabase.setDatabaseName(m_databaseName);//设置数据库文件名
    /*m_sqlDatabase.setHostName();设置主机名
    m_sqlDatabase.setPort();设置端口
    m_sqlDatabase.setUserName();设置用户名
    m_sqlDatabase.setPassword();设置密码*/
    if (!m_sqlDatabase.isValid())//判断是否有可用驱动
    {
        log << "错误：数据库类型QSQLITE不可用，请检查Qt中对应的驱动";
    }
    else
    {
        log << "已添加类型为QSQLITE的数据库连接: " << m_sqlDatabase.databaseName();
    }
}
DatabaseManager::DatabaseManager(const DatabaseManager&)
{
}

DatabaseManager& DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::init()//初始化操作主要是打开数据库，初始化歌曲列表
{
    bool ret = m_sqlDatabase.open();
    if(!ret)
    {
        log << "打开失败: " << m_sqlDatabase.lastError().text();
        return  ret;
    }
    ret =initSong();

    return ret;
}

bool DatabaseManager::initSong()//初始化歌曲表
{
    bool ret = false;

    QSqlQuery query;//执行数据库语句 使用QSqlQuery
    QString sql = "create table if not exists songs(";
    sql += "id integer primary key,";
    sql += "url text unique not null,";
    sql += "name text,";
    sql += "artist text,";
    sql += "album text);";
    log << sql;
    ret = query.exec(sql);
    if(!ret)
    {
        log << "创建歌曲表失败：" << query.lastError().text();
        return ret;
    }
    log << "创建歌曲表成功";

    return ret;
}

//析构时移除数据库m_databaseName
DatabaseManager::~DatabaseManager()
{
    destroy();
}

void DatabaseManager::destroy()//数据库定义释放接口
{
    if (m_sqlDatabase.isOpen())
    {
        m_sqlDatabase.close();//关闭数据库
    }

    if (m_sqlDatabase.isValid())
    {
        m_sqlDatabase.removeDatabase(m_databaseName);//删除数据库默认连接。
    }
}

bool DatabaseManager::addSong(const Song& song)
{
    bool ret = m_sqlDatabase.isOpen();
    if(!ret)
    {
        log << "数据库未打开，无法添加歌曲";
        return  ret;
    }
    QSqlQuery query;//执行数据库语句 使用QSqlQuery
    QString sql = "insert into songs(url, name, artist, album) values (";
    sql += "'" + song.url().toString() + "',"; //使用单引号包含字符串变量，避免为空
    sql += "'" + song.name() + "',";
    sql += "'" + song.artist() + "',";
    sql += "'" + song.album() + "');";
    log << sql;
    ret = query.exec(sql);
    if(!ret)
    {
        log << "添加歌曲失败: " << query.lastError().text();
        return ret;
    }
    log << "添加歌曲成功";
    return ret;
}

bool DatabaseManager::querySongs(QVector<Song *> &songsResult)//恢复上次播放列表
{
    bool ret = m_sqlDatabase.isOpen();
    if(!ret)
    {
        log << "数据库未打开，无法添加歌曲";
        return ret;
    }
    QSqlQuery query;//执行数据库语句 使用QSqlQuery
    QString sql = "select * from songs;";
    query.exec(sql);
    if (!ret)
    {
        log << "查询歌曲失败，错误信息：" << query.lastError().text();
        return ret;
    }
    log << "查询歌曲成功" << endl;
    //处理查询结果
    QUrl url;
    QString name, artist, album;
    while (query.next()) //获取一条记录，存储到query中
    {
        url = QUrl(query.value("url").toString());
        name = query.value("name").toString();
        artist = query.value("artist").toString();
        album = query.value("album").toString();
        songsResult.push_back(new Song(url, name, artist, album));
    }
    return ret;
}

bool DatabaseManager::clearAllSong()
{
    bool ret = m_sqlDatabase.isOpen();
    if(!ret)
    {
        log << "数据库未打开，无法添加歌曲";
        return ret;
    }
    QSqlQuery query;//执行数据库语句 使用QSqlQuery
    //不能使用clear
    QString sql = "delete from songs;";
    ret = query.exec(sql);
    if(!ret)
    {
        log << "清空歌曲失败，删除语句执行失败: " << query.lastError().text();
        return ret;
    }
    log << "清空歌曲成功";
    return ret;
}
