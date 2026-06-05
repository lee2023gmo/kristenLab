#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QString>

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    bool play(const QString &filePath, bool loop = true);
    void stop();
    void setVolume(float volume);

    QString lastError() const;

private:
    struct Impl;
    Impl *d;
};

#endif // AUDIOPLAYER_H
