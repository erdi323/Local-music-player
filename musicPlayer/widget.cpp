#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>//文件对话框
#include <QUrl>
#include <QTextCodec>//处理编码
#include "utils.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_db(DatabaseManager::getInstance())//数据库管理对象引用成员初始化
{
    ui->setupUi(this);
    this->setWindowTitle("Music Player");//修改窗口名

    m_player = new QMediaPlayer(this);//初始化媒体播放对象

    //连接和处理后台媒体播放器的位置变化信号
    QObject::connect(m_player,&QMediaPlayer::positionChanged,//播放位置已更改
                     this, &Widget::handlePositionChanged);

    m_list = new QMediaPlaylist(this);//初始化后台播放列表对象
    m_player->setPlaylist(m_list);//将媒体播放列表对象设置给媒体播放器，让它自动从列表中读取歌曲进行播放
    m_list->setPlaybackMode(QMediaPlaylist::Loop); //设置默认播放模式为循环播放
    //ui->pushButton_mode->setText("循环播放");文本换图标

    //连接和处理媒体播放列表的当前媒体变化信号
    QObject::connect(m_list,&QMediaPlaylist::currentMediaChanged,//当前媒体已更改
                     this,&Widget::handlePlaylistCurrentMediaChanged);

    //连接和处理媒体播放列表的播放模式变化信号
    QObject::connect(m_list,&QMediaPlaylist::playbackModeChanged,//播放模式已更改
                     this,&Widget::handlePlaylistPLaybackModeChanged);

    style();

    QTimer * m_timer = new QTimer;

    QObject::connect(m_timer,&QTimer::timeout,
                     this,&Widget::handleTimeout);
    m_timer->start(100);//指定100毫秒触发超时信号

    QObject::connect(m_player,&QMediaPlayer::stateChanged,
                     this,&Widget::handlePlayerStateChanged);
    initToSql();
    queryAllSongsFromSql();
}

Widget::~Widget()
{
    delete ui;
    for (auto eachValue: m_currentPlaylist)//历遍容器的每一个value及song*，进行释放
    {
        delete eachValue;
    }
    m_currentPlaylist.clear();

}

/*添加音乐
1.打开文件对话框，选择mp3文件
2.如果返回文件，就设置给后台媒体播放器
3.将音乐文件添加到媒体播放列表中，取消添加到播放器
4.所播放歌曲名的显示，通过连接和接收媒体播放列表的当前媒体变化信号，从中读取当前音乐媒体，并显示歌曲名
5.所添加歌曲名的显示，构造一个列表控件的元素，设置给前端播放列表控件*/
void Widget::on_pushButton_addMiusic_clicked()
{
    //QString file = QFileDialog::getOpenFileName(  单个获取
    QStringList files = QFileDialog::getOpenFileNames(this,//多个获取
                                                "添加音乐",//对话框标题
                                                "",//打开路径
                                                "mp3文件(*.mp3)");//指定文件样式

    if (files.isEmpty())
    {
        log << "file is empty";
    }
    for (auto file:files)//自动获取遍历
    {
        log << "添加音乐文件：" << file ;
        //针对文件路径字符串做转换，防止后续使用过程中路径字符串存在差异
        file = QMediaContent(QUrl(file)).request().url().path();
        //先查询歌曲对象容器中是否已有该歌曲文件，避免重复添加
        if (m_currentPlaylist.contains(file))
        {
            log << "歌曲已存在：" << file;
            continue;
        }

        m_list->addMedia(QMediaContent(QUrl(file)));//设置给后台媒体播放器

        const Song * song = getSongInfoFromMp3File(file);//获得歌曲信息
        QString text = song->name() + " - " + song->artist();//解析后通过歌曲和歌手添加
        addSongToSql(*song);
        //构造一个列表控件的元素，设置给前端播放列表控件
        QListWidgetItem * item = new QListWidgetItem();
        //item->setText(QFileInfo(file).baseName());//通过歌名
        item->setText(text);
        ui->listWidget_list->addItem(item);
    }
    m_player->play();//播放
}

/*
解析mp3文件，获取其中的歌曲信息
逻辑：
1.打开文件，跳转到倒数第128个字节处
2.读取最后128字节
将字节[3, 32]范围里（即偏移量为3到32的字节数据）的数据解析成歌曲名
将字节[33, 62]范围里的数据解析成歌手名
将字节[63, 92]范围里的数据解析成专辑名
3.构造一个歌曲对象，保存到歌曲对象容器中
4.保存到数据库中
*/
const Song * Widget::getSongInfoFromMp3File(const QString& filepath)//获得歌曲信息
{
    log << filepath;
    QString name, artist, album;
    QFile qfile(filepath);
    if(qfile.open(QIODevice::ReadOnly | QIODevice::Text))//以只读文本打开
    {
        int length = qfile.size();//获取长度
        qfile.seek(length - 128);//跳转到倒数第128字节处
        char buffer[128 + 1] = {0};//使用char数组存储所读取的信息
        int readSize = qfile.read(buffer,sizeof(buffer) - 1 );//read读取后返回读取的数量
        if(readSize > 0) //sizeof(buffer) - 1 = 128
        {
            //log << readSize;
            //从数组中取3个字节，从8位即一个字节表示一个字符char的格式进行转换，
            //构造一个QString（16位即两个字节表示一个字符QChar）
            QString tag = QString::fromLocal8Bit(buffer + 0, 3);//起始位置与读取个数
            //QString tag1 = QString::fromUtf8(buffer, 3);
            name = QString::fromLocal8Bit(buffer + 3, 30);//获取歌曲名，从位置buffer + 3，取30个字节
            artist = QString::fromLocal8Bit(buffer + 33, 30);//歌手
            album = QString::fromLocal8Bit(buffer + 63, 30);//专辑
            //去掉多余的空字符
            name.truncate(name.indexOf(QChar::Null)); //name.truncate(name.indexOf(char(NULL)));
            artist.truncate(artist.indexOf(QChar::Null));//artist.truncate(artist.indexOf(char(NULL)));
            album.truncate(album.indexOf(QChar::Null));//album.truncate(album.indexOf(char(NULL)));
            log << tag << name << "，" << artist << "，" << album;
        }
        else
        {
            log << "读取文件失败";
        }
    }
    qfile.close();
    //3.构造一个歌曲对象，保存到歌曲对象容器中
    Song * song = new Song(QUrl(filepath),name,artist,album);
    this->m_currentPlaylist.insert(filepath, song);
    return song;
}

void Widget::on_pushButton_play_paus_clicked()
{
    if(m_player->state() != QMediaPlayer::PlayingState)//判断是否为播放状态
    {
        m_player->play();//播放
        //ui->pushButton_play_paus->setText("暂停");
    }
    else
    {
        m_player->pause();//暂停
        //ui->pushButton_play_paus->setText("播放");
    }
}

void Widget::handlePlayerStateChanged(QMediaPlayer::State state)//播放图标替换
{
    if(state == QMediaPlayer::PlayingState)
    {
        ui->pushButton_play_paus->setIcon(QIcon(":/icons/play.png"));
    }
    else
    {
        ui->pushButton_play_paus->setIcon(QIcon(":/icons/pause.png"));
    }

}

//自定义槽函数，连接和处理后台媒体播放器的位置变化信号
void Widget::handlePositionChanged(qint64 position)
{
    qint64 duration = m_player->duration();//后台歌曲时长
    int slidermax = ui->horizontalSlider->maximum();//进度条的最大值
    //前台滑动条的值 = 后台当前播放位置 / 后台歌曲时长 * 前端进度条最大值
    ui->horizontalSlider->setValue((double)position/duration * slidermax);
   // log << position;
    QString text;
    /*处理补0: (数值, 位数-字符个数, 进制-使用十进制10, 填充字符-使用0)
    后台当前分钟数 = 以秒为单位的当前播放位置 / 60
    后台当前秒数 = 以秒为单位的当前播放位置 % 60
    歌曲时长的分钟数 = 以秒为单位的歌曲时长 / 60
    歌曲时长的秒数 = 以秒为单位的歌曲时长 % 60
    positions 是以毫秒为单位 不能直接%60， 需先%60000再/1000*/
    text = QString("%1").arg(position / 60000,2,10,QChar('0'));
    text += ":";
    text += QString("%1").arg(position % 60000 /1000,2,10,QChar('0'));
    text += "/";
    text += QString("%1").arg(duration / 60000,2,10,QChar('0'));
    text += ":";
    text += QString("%1").arg(duration % 60000 /1000,2,10,QChar('0'));
    ui->label_playTime->setText(text);//显示播放进度
    /*log << "position: " << position
        << ", sliderMax: "<< slidermax
        << ", play time: " << text;*/
}

/*
函数：读取歌词
参数：file：歌词文件路径
描述：从歌词文件中读取并解析歌词内容，并存储到歌词存储容器中
逻辑：
0.清空歌词存储容器
1.判断参数是否是一个文件
2.读取文件
2.1 使用QFile打开文件
2.2 使用QTextStream按行读取文件内容
    声明一个字符串line，保存每行内容
    声明一个字符串列表lineContents，保存第一次分割后的行内容 {"[分钟数:秒数.毫秒数", "歌词文本"}
    声明一个字符串列表timeContents，保存第二次分割后的时间内容 {"[分钟数", "秒数.毫秒数"}
    读取一行内容，判断行内容是否为空，为空就下一次循环，否则解析内容，并存储到歌词容器中

2.2.1 解析行内容
    行文本格式："[时间标签]文本"
    时间标签格式："[分钟数:秒数.毫秒数]"

    1）根据']'第一次分割行内容，得：
      A [分钟数:秒数.毫秒数
      B 歌曲信息或歌词文本

    2）根据':'第二次分割，分割A得：
      A1 [分钟数
      A2 秒数.毫秒数

    3）A1去掉'['，转换成整型的分钟数，mid截取偏移量为1处开始的子串
    4）A2转换成浮点型的秒数
    5）(分钟数 * 60 + 秒数) * 1000 = 这一行对应的播放时间进度（毫秒级）
    6）B文本可以直接存储和使用
2.2.2 存储每行歌词内容到歌词容器
*/
void Widget::readLyricsFromFile(const QString& file)//从歌词文件中读取并解析歌词内容
{
    m_lyrics.clear();
    bool ret = QFileInfo(file).isFile();
    if (!ret)
    {
        log << "it is not a file" << file;
        return;
    }
    log << "read lyrics from file: " << file;//读取file的歌词
    QFile qfile(file);//构建文件对象
    ret = qfile.open(QIODevice::ReadOnly | QIODevice::Text);//只读和文本
    if (!ret)
    {
        log << "open file failed:" << file;
        return;
    }
    log << "open file successfully";

    QTextStream ts(&qfile);//声明一个文件流对象
    QString line;//保存每行内容
    QStringList lineContents; //保存第一次分割后的行内容 {"[分钟数:秒数.毫秒数", "歌词文本"}
    QStringList timeContents; //保存第二次分割后的时间内容 {"[分钟数", "秒数.毫秒数"}

    QByteArray array = qfile.readAll();//声明一个字节数组类储存文件内容
    //log << array.constData();//与data相似 返回char指针 指向所指内容
    //log << array.data();
    qfile.seek(0);//跳回最前端

    QTextCodec::ConverterState state;// QTextCodec可以用来转换某些局部编码的字符串成为统一码
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");

    //将此编解码器编码输入中的第一个大小字符转换为 Unicode(统一码），并在 QString 中返回结果。
    codec->toUnicode(array.constData(), array.size(), &state);

    QStringList ts1 = QString(array.constData()).split('\n');//转换为Qstring分隔
    QListIterator<QString> itr(ts1);//迭代器方式1
   /* for(auto it = ts1.begin();it != ts1.end(); it++)迭代器方式2
    {
        //log << *it;
    }*/
    log << state.invalidChars;
    while (!ts.atEnd() && itr.hasNext()) //it != ts1.end())
        //bool QListlterator::hasNext() const如果迭代器前面至少有一项，即迭代器不在容器的后面，则返回true;否则返回false。
    {
        if(state.invalidChars > 0)//判断是否有无效字符,没有为utf-8编码
        {
            line = ts.readLine();
        }
        else
        {
            //line = *it;++it;
            line = itr.next();
        }

        if (line.isEmpty())
        {
            continue;//跳过这个循环
        }

        log << "line" << line;
        /*根据']'第一次分割行内容，得时间标签和文本：
        lineContents[0] A [分钟数:秒数.毫秒数
        lineContents[1] B 歌曲信息或歌词文本*/
        lineContents = line.split(']');//分隔

        /*根据':'第二次分割时间标签A [分钟数:秒数.毫秒数，得
        timeContents[0] A1 [分钟数
        timeContents[1] A2 秒数.毫秒数*/
        timeContents = lineContents[0].split(':');

        //A1去掉'['，转换成整型的分钟数，mid截取偏移量为1处开始的子串
        int minutes = timeContents[0].mid(1).toInt();

        //A2转换成浮点型的秒数
        double seconds = timeContents[1].toDouble();

        /*(分钟数 * 60 + 秒数) * 1000 = 这一行对应的播放时间进度（毫秒级）
        B文本可以直接存储和使用*/
        m_lyrics.insert((minutes * 60 + seconds) * 1000, lineContents[1]);

        lineContents.clear();
        timeContents.clear();
    }
}

void Widget::updateAllLyrics()//显示全部歌词函数
{
    log;
    for (auto text : m_lyrics.values())
    {
        QListWidgetItem * item = new QListWidgetItem(text, ui->listWidget_lyrics);
        item->setTextAlignment(Qt::AlignCenter); //设置文本居中
    }
}

void Widget::on_horizontalSlider_sliderReleased()//滑动条显示进度
{
    //后台当前播放位置 = 滑动条的值 / 滑动条最大值 * 后台歌曲时长
    int sliderValue = ui->horizontalSlider->value();
    int sliderMaxValue = ui->horizontalSlider->maximum();
    m_player->setPosition((double)sliderValue/sliderMaxValue * m_player->duration());
}

//连接和处理后台媒体播放器的位置变化信号
void Widget::handlePlaylistCurrentMediaChanged(const QMediaContent& content)
{
    ui->listWidget_list->setCurrentRow(m_list->currentIndex());//高亮播放歌曲
    if(m_player->state() != QMediaPlayer::PlayingState)//判断播放状态
    {
        return;
    }
    if(content.isNull())
    {
        log << "当前媒体数据为空,跳转到第一首";
        m_list->next();//空下一首是循环
        return;
    }
    QString fileName =  content.request().url().fileName();
    log << fileName;//带后缀的文件名
    ui->label_songName->setText(QFileInfo(fileName).baseName());//显示不带后缀的文件名
    /*处理媒体播放列表的当前媒体(歌曲)变化信号
    1.显示歌曲名到界面（实际上是文件名）
    2.获取歌曲文件.mp3路径，替换后缀.lrc得歌词文件路径
    3.调用读取歌词文件函数
    4.更新界面歌词显示*/
    ui->listWidget_lyrics->clear();
    QString mediaFilePath = content.request().url().path();//媒体文件路径
    QString lyricsFilePath = mediaFilePath.replace(".mp3",".lrc");//歌词文件路径
    //QString lyricsFilePath = content.request().url().path().replace(".mp3",".lrc");
    log << lyricsFilePath;
    if (QFileInfo(lyricsFilePath).isFile())
    {
        log << "it is a file";
        readLyricsFromFile(lyricsFilePath);//调用读取
        updateAllLyrics();
    }
    else
    {
        log << "it is not a file";
        ui->listWidget_lyrics->addItem("无歌词文件");
    }
}

void Widget::on_pushButton_top_clicked()
{
    m_list->previous();//上一首
    m_player->play();
}

void Widget::on_pushButton_next_clicked()
{
    m_list->next();//下一首
    m_player->play();
}

/*点击时切换播放模式， 切换为下一一个播放模式{0， 1, 2, 3, 4}
enum PlaybackMode { CurrentI temonce, CurrentItemInLoop, Sequential, Loop, Random };
获取当前模式    计算下一个模式的值(取余)   设置给后台播放列表*/
void Widget::on_pushButton_mode_clicked()
{
    QMediaPlaylist::PlaybackMode mode = m_list->playbackMode();//获取当前播放模式
    int nextMode =(mode + 1) % 5;//每次按下加1
    log << "mode:" << mode << "nextMode:" << nextMode;
    m_list->setPlaybackMode(QMediaPlaylist::PlaybackMode(nextMode));//设置播放模式
}

//处理媒体播放列表的播放模式变化信号 根据当前模式显示对应文本
void Widget::handlePlaylistPLaybackModeChanged(QMediaPlaylist::PlaybackMode mode)
{
    log << mode;
    //static QStringList texts = {"单曲播放","单曲循环","顺序播放","循环播放","随机播放"};
    //ui->pushButton_mode->setText(texts[mode]);

    //文本替换成图标
    static QStringList iocns = {":/icons/currentItemOnce.png",
                                ":/icons/currentItemLoop.png",
                                ":/icons/sequential.png",
                                ":/icons/loop.png",
                                ":/icons/random.png"
                               };
    ui->pushButton_mode->setIcon(QIcon(iocns[mode]));
}
void Widget::handleTimeout()//处理超时信号
{
    updateLyricsOnTime();
}

/*
实时刷新歌词，实现歌词同步，同步所播放的当前行歌词，针对当前行歌词进行高亮和滚动垂直居中显示
1.判断是否有歌词
2.如果界面列表控件还没有开始同步显示当前行(currentRow默认-1)，也就是刚开始播放时显示了所有歌词，
     就将歌词列表控件的第一行设置为当前行，当前行的效果是高亮显示
3.读取媒体播放器的当前播放时间进度（毫秒级时刻）和当前行数
4.比较媒体播放器的当前播放时间和界面列表当前行的起始播放时间戳（毫秒级时刻）
4.1 如果当前播放时间小于当前行的起始播放时间戳
    往前找匹配的歌词行，找到当前播放时间大于或等于某一行的起始播放时间戳，
    即这一行就是当前应该同步显示的行，记录这个行的索引，即行数

4.2 如果当前播放时间大于当前行的起始播放时间戳
    往后找匹配的歌词行，找到播放时间小于某一行的起始播放时间戳，
    即它的上一行是所要同步显示的行，记录这个行的索引，即行数

4.3 如果等于，就是这行，已同步

5.将所要同步显示行进行高亮和滚动垂直居中显示
5.1 获取列表控件的当前行元素
5.2 设置当前行，高亮显示当前所同步的行元素
5.3 当前行元素滚动到垂直居中位置（滚动居中、居顶、居底，自动滚动）
 */
void Widget::updateLyricsOnTime()//实时刷新歌词函数
{
    if(m_list->isEmpty())//如果没有歌，不运行
    {
        return;
    }
    if(m_player->state() != QMediaPlayer::PlayingState)
    {
        return;
    }
    if(m_lyrics.isEmpty())
    {
        log << "歌词为空";
        return;
    }
    if(ui->listWidget_lyrics->currentRow() == -1)
    {
        ui->listWidget_lyrics->setCurrentRow(0);//设置第一行为当前行，高亮显示
        return;
    }
    qint64 position = m_player->position(); //播放进度，毫秒级时刻
    int currentRow = ui->listWidget_lyrics->currentRow(); //当前行的行数
    QList<qint64> lyricsTimeStamp = m_lyrics.keys(); //获取歌词容器的时间戳部分keys: {time1, time2, time3}
    //log << m_lyrics.keys();key在237行
    if (position < lyricsTimeStamp[currentRow])//如果播放器进度小于当前行的起始播放时间，即还没播放到这一行来，就往前找匹配的行
    {
        while(currentRow > 0) //只要前面还有行，就往前找匹配播放时间的歌词行
        {
            --currentRow;//这一行不匹配，递减往前找，先递减后判断
            if (position >= lyricsTimeStamp[currentRow])//找到匹配的行，即播放器进度大于或等于该行起始播放时间的，后续用来进行同步
            {
                break;
            }
        }
    }
    else if (position > lyricsTimeStamp[currentRow])//如果播放器进度大于当前行的起始播放时间，即放到了这一行或以后的行，往后确定匹配行
    {
        while(currentRow < lyricsTimeStamp.size() - 1) //只要后面还有行，就往后找匹配播放时间的歌词行
        {
            if (position < lyricsTimeStamp[currentRow + 1])//找到匹配的行，即播放器进度小于下一行起始播放时间
            {
                break;
            }
            ++currentRow;//这一行不匹配，递增往后找，先判断后递增
        }
    }
    else {}

    QListWidgetItem * item = ui->listWidget_lyrics->item(currentRow);
    ui->listWidget_lyrics->setCurrentRow(currentRow);//设置当前行高亮
    ui->listWidget_lyrics->scrollToItem(item,QAbstractItemView::PositionAtCenter);//设置滚动居中
}

void Widget::on_pushButton_clear_clicked()//清空歌单
{
    m_list->clear();
    ui->label_songName->setText("Song");
    ui->listWidget_list->clear();
    ui->listWidget_lyrics->clear();
    for (auto eachValue: m_currentPlaylist)//历遍容器的每一个value及song*，进行释放
    {
        delete eachValue;
    }
    m_currentPlaylist.clear();
    clearAllSongFormSql();
}

void Widget::on_listWidget_list_itemDoubleClicked(QListWidgetItem *item)//双击切歌
{
    m_list->setCurrentIndex(ui->listWidget_list->currentRow());
    m_player->play();
}

//数据库相关
void Widget::initToSql()//打开数据库
{
    if(!m_db.init())
    {
        log << "数据库不可用";
        QMessageBox::warning(this,"持久化","数据库不可用");
    }
}

void Widget::addSongToSql(const Song & song)//添加歌曲
{

    if(!m_db.addSong(song))
    {
        QMessageBox::warning(this,"持久化","添加歌曲失败");
    }
}

void Widget::queryAllSongsFromSql()//恢复上次播放列表
{
    QVector<Song*> songs;
    bool ret = m_db.querySongs(songs);

    log << "查询到歌曲数量：" << songs.size();
    //TODO: 处理查询结果，展示到界面
    //DO 2022.11.13
    if (!ret)
    {
        log << "查询失败";
        return;
    }

    for (auto song : songs)
    {
        //log << song->url();log << song->url().path();
        log << song->url().toString();
        m_currentPlaylist.insert(song->url().path(), song);
        m_list->addMedia(song->url());
        ui->listWidget_list->addItem(new QListWidgetItem(song->name() +  " - " + song->artist()));
    }
}

void Widget::clearAllSongFormSql()//删除歌曲表
{
    m_db.clearAllSong();
}

void Widget::style()//样式
{
    //布局
    QHBoxLayout * hb1 = new QHBoxLayout;//水平布局
    hb1->addWidget(ui->pushButton_addMiusic);
    hb1->addWidget(ui->pushButton_clear);

    QVBoxLayout * vb1 = new QVBoxLayout;//垂直布局
    vb1->addLayout(hb1,1);
    vb1->addWidget(ui->listWidget_list,5);

    QHBoxLayout * hb2 = new QHBoxLayout;//水平布局
    hb2->addWidget(ui->listWidget_lyrics,2);
    hb2->addLayout(vb1,1);

    QHBoxLayout * hb3 = new QHBoxLayout;//水平布局
    hb3->addWidget(ui->label_songName);

    QHBoxLayout * hb4 = new QHBoxLayout;//水平布局
    hb4->addWidget(ui->horizontalSlider);
    hb4->addWidget(ui->label_playTime);

    QHBoxLayout * hb5 = new QHBoxLayout;//水平布局
    hb5->addWidget(ui->pushButton_top);
    hb5->addWidget(ui->pushButton_play_paus);
    hb5->addWidget(ui->pushButton_next);
    hb5->addWidget(ui->pushButton_mode);

    QVBoxLayout * vb2 = new QVBoxLayout;//垂直布局
    vb2->addLayout(hb2,6);
    vb2->addLayout(hb3,1);
    vb2->addLayout(hb4,1);
    vb2->addLayout(hb5,2);

    //窗口
    this->setStyleSheet("background-color: rgb(52, 152, 219)");

    //列表和歌词
    //ui->listWidget_lyrics->setStyleSheet("background-color: transparent");//背景透明
    ui->listWidget_lyrics->setStyleSheet("background-color:rgb(162, 217, 206)");
    ui->listWidget_lyrics->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//设置垂直滚动条策略
    ui->listWidget_lyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//设置水平滚动条策略(滚动条始终关闭)
    ui->listWidget_lyrics->setFrameShape(QListWidget::NoFrame);//设置框架形状(无边框)

    ui->listWidget_list->setStyleSheet("background-color: rgb(162, 217, 206)");
    ui->listWidget_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_list->setFrameShape(QListWidget::NoFrame);
    //按钮
    ui->pushButton_addMiusic->setStyleSheet("background-color: rgb(249, 231, 159)");
    ui->pushButton_clear->setStyleSheet("background-color: rgb(249, 231, 159)");
    ui->pushButton_top->setStyleSheet("background-color: rgb(249, 231, 159)");
    ui->pushButton_play_paus->setStyleSheet("background-color: rgb(249, 231, 159)");
    ui->pushButton_next->setStyleSheet("background-color: rgb(249, 231, 159)");
    ui->pushButton_mode->setStyleSheet("background-color: rgb(249, 231, 159)");

    this->setLayout(vb2);
}
