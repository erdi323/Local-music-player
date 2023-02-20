#ifndef SONG_H
#define SONG_H

//歌曲类
//1.封装媒体文件路径url、歌曲名name、歌手artist、专辑名album等歌曲信息作为数据成员
//2.提供相关接口访问这些数据成员
//在添加音乐时解析.mp3文件获取这些信息，构造一个歌曲对象存储在主窗口的歌曲存储容器里m_currentPlaylist;

#include <QUrl>
class Song
{
public:
    Song();
    Song(const QUrl& url) : m_url(url) {}
    Song(const QUrl& url,
         const QString& name,
         const QString& artist,
         const QString& album)
        : m_url(url),
          m_name(name),
          m_artist(artist),
          m_album(album) {}

    QUrl url() const { return m_url; }
    QString name() const { return m_name; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }

private:
    QUrl m_url;//路径
    QString m_name;//歌曲名
    QString m_artist;//歌手
    QString m_album;//专辑名

};

#endif // SONG_H
