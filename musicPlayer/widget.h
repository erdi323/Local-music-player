#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlayer>//媒体播放器类头文件    需要.pro配置文件引用模块multimedia
#include <QMediaPlaylist>//后台播放列表类头文件    需要.pro配置文件引用模块multimedia
#include <QTimer>
#include "song.h"
#include "databasemanager.h"
#include <QListWidgetItem>


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void readLyricsFromFile(const QString& file);//从歌词文件中读取并解析歌词内容

    void updateAllLyrics();//显示全部歌词函数

    void updateLyricsOnTime();//实时刷新歌词函数

    const Song* getSongInfoFromMp3File(const QString& file);//获得歌曲信息

    void style();//样式

    //////////////////////////////////////////////////////
    //数据库相关
    void initToSql();//打开数据库

    void addSongToSql(const Song & song);//添加歌曲

    void queryAllSongsFromSql();//恢复上次播放列表

    void clearAllSongFormSql();//删除歌曲表
    /////////////////////////////////////////////////////
private slots:
    void on_pushButton_addMiusic_clicked();//添加音乐

    void on_pushButton_play_paus_clicked();//播放与暂停

    void on_horizontalSlider_sliderReleased();//进度条

    //自定义槽函数，连接和处理后台媒体播放器的位置变化信号
    void handlePositionChanged(qint64 position);//传入当前播放位置

    //自定义槽函数，连接和接收媒体播放列表的当前媒体变化信号
    void handlePlaylistCurrentMediaChanged(const QMediaContent& content);//传入媒体内容

    void on_pushButton_top_clicked();//上一首

    void on_pushButton_next_clicked();//下一首

    //自定义槽函数，连接和处理媒体播放列表的播放模式变化信号
    void handlePlaylistPLaybackModeChanged(QMediaPlaylist::PlaybackMode mode);//传入播放模式

    void on_pushButton_mode_clicked();//播放模式切换

    void handleTimeout();//自定义槽函数，处理超时信号

    void on_pushButton_clear_clicked();//清空歌单

    //自定义槽函数，处理播放状态
    void handlePlayerStateChanged(QMediaPlayer::State state);//传入播放状态

    void on_listWidget_list_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::Widget *ui;
    QMediaPlayer * m_player;//声明数据成员-后台的媒体播放器对象
    QMediaPlaylist * m_list;//声明数据成员-后台播放列表对象
    QMap<qint64,QString> m_lyrics;//存储一首歌的歌词
    QTimer *m_timer;//声明一个定时器-处理界面歌词同步的定时器，定时刷新歌词

    QMap<QString, Song*>  m_currentPlaylist; //声明存储当前播放列表的键值对容器，存储所添加的音乐

    DatabaseManager& m_db;//数据库管理对象的引用
};
#endif // WIDGET_H
