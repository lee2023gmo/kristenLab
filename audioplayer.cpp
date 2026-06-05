#include "audioplayer.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>

struct AudioPlayer::Impl {
    ma_engine engine;
    ma_sound sound;
    bool engineInitialized;
    bool soundInitialized;
    QString errorString;

    Impl()
        : engineInitialized(false)
        , soundInitialized(false)
    {
    }

    ~Impl()
    {
        if (soundInitialized) {
            ma_sound_uninit(&sound);
        }
        if (engineInitialized) {
            ma_engine_uninit(&engine);
        }
    }
};

AudioPlayer::AudioPlayer()
    : d(new Impl())
{
}

AudioPlayer::~AudioPlayer()
{
    delete d;
}

bool AudioPlayer::play(const QString &filePath, bool loop)
{
    d->errorString.clear();

    if (!d->engineInitialized) {
        ma_result result = ma_engine_init(nullptr, &d->engine);
        if (result != MA_SUCCESS) {
            d->errorString = QString("音频引擎初始化失败 (错误码: %1)").arg(result);
            return false;
        }
        d->engineInitialized = true;
    }

    if (d->soundInitialized) {
        ma_sound_uninit(&d->sound);
        d->soundInitialized = false;
    }

    // 如果路径包含非 ASCII 字符（如中文），miniaudio 的 fopen 可能失败。
    // 先把文件复制到一个纯 ASCII 的临时路径，确保能打开。
    QString pathToUse = filePath;
    QTemporaryFile tempFile;
    bool usingTempFile = false;

    for (const QChar &c : filePath) {
        if (c.unicode() > 127) {
            // 路径包含非 ASCII 字符，使用临时文件
            tempFile.setFileTemplate(QDir::temp().filePath("kristenlab_bgm_XXXXXX.mp3"));
            if (tempFile.open()) {
                tempFile.close();
                if (QFile::copy(filePath, tempFile.fileName())) {
                    pathToUse = tempFile.fileName();
                    usingTempFile = true;
                }
            }
            break;
        }
    }

    QByteArray pathUtf8 = pathToUse.toUtf8();
    ma_result result = ma_sound_init_from_file(&d->engine, pathUtf8.constData(),
                                               MA_SOUND_FLAG_STREAM, nullptr, nullptr, &d->sound);

    // 如果 UTF-8 路径失败，再尝试本地编码路径
    if (result != MA_SUCCESS) {
        QByteArray pathLocal = QDir::toNativeSeparators(pathToUse).toLocal8Bit();
        if (pathLocal != pathUtf8) {
            result = ma_sound_init_from_file(&d->engine, pathLocal.constData(),
                                             MA_SOUND_FLAG_STREAM, nullptr, nullptr, &d->sound);
        }
    }

    if (result != MA_SUCCESS) {
        d->errorString = QString("无法打开音频文件 (错误码: %1)。\n尝试路径: %2")
                             .arg(result)
                             .arg(pathToUse);
        if (usingTempFile) {
            QFile::remove(tempFile.fileName());
        }
        return false;
    }

    d->soundInitialized = true;
    ma_sound_set_looping(&d->sound, loop ? MA_TRUE : MA_FALSE);

    result = ma_sound_start(&d->sound);
    if (result != MA_SUCCESS) {
        d->errorString = QString("音频启动失败 (错误码: %1)").arg(result);
        return false;
    }

    return true;
}

void AudioPlayer::stop()
{
    if (d->soundInitialized) {
        ma_sound_stop(&d->sound);
    }
}

void AudioPlayer::setVolume(float volume)
{
    if (d->soundInitialized) {
        ma_sound_set_volume(&d->sound, volume);
    }
}

QString AudioPlayer::lastError() const
{
    return d->errorString;
}
