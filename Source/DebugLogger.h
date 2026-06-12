#pragma once
#include <juce_core/juce_core.h>
#include <fstream>
#include <queue>

class DebugLogger : private juce::Thread
{
public:
    DebugLogger(const juce::String& filePath) : Thread("DebugLogger"), file(filePath) 
    {
        startThread();
    }

    ~DebugLogger() { stopThread(2000); }

    // Call this from the audio thread - it is non-blocking!
    void log(const juce::String& message)
    {
        const juce::ScopedLock sl(queueLock);
        messageQueue.push(juce::Time::getCurrentTime().toString(true, true) + ": " + message);
    }

private:
    void run() override
    {
        std::ofstream outfile(file.getFullPathName().toStdString(), std::ios_base::app);
        while (!threadShouldExit())
        {
            if (!messageQueue.empty())
            {
                juce::String msg;
                {
                    const juce::ScopedLock sl(queueLock);
                    msg = messageQueue.front();
                    messageQueue.pop();
                }
                outfile << msg.toStdString() << std::endl;
            }
            wait(10); // Don't hog CPU
        }
    }

    juce::File file;
    juce::CriticalSection queueLock;
    std::queue<juce::String> messageQueue;
};