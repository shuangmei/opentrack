#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include <QThread>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSettings>

#include "qxt-mini/QxtGlobalShortcut"
#include "opentrack/plugin-support.h"
#include "opentrack/options.hpp"
#include "ui_ftnoir_keyboardshortcuts.h"

using namespace options;

extern QList<QString> global_key_sequences;

struct key_opts {
    value<int> key_index;
    value<bool> ctrl, alt, shift;

    key_opts(pbundle b, const QString& name) :
        key_index(b,  QString("key-index-%1").arg(name), 0),
        ctrl(b,  QString("key-ctrl-%1").arg(name), 0),
        alt(b,  QString("key-alt-%1").arg(name), 0),
        shift(b,  QString("key-shift-%1").arg(name), 0)
    {}
};

#if defined(_WIN32)
extern QList<int> global_windows_key_sequences;
#   undef DIRECTINPUT_VERSION
#   define DIRECTINPUT_VERSION 0x0800
#   include <windows.h>
#   include <dinput.h>

struct Key {
    BYTE keycode;
    bool shift;
    bool ctrl;
    bool alt;
    bool ever_pressed;
    QElapsedTimer timer;
public:
    Key() : keycode(0), shift(false), ctrl(false), alt(false), ever_pressed(false)
    {
    }
};
#else
typedef unsigned char BYTE;
struct Key { int foo; };
#endif

struct KeybindingWorker : public QThread {
    Q_OBJECT
#ifdef _WIN32
private:
    LPDIRECTINPUT8 din;
    LPDIRECTINPUTDEVICE8 dinkeyboard;
    Key kCenter;
    Key kToggle;
public:
    volatile bool should_quit;
    ~KeybindingWorker();
    KeybindingWorker(Key keyCenter, Key keyToggle, WId handle);
	void run();
#else
public:
    KeybindingWorker(Key, Key, WId) {}
	void run() {}
#endif
signals:
    void center();
    void toggle();
};


struct Shortcuts {
    using K =
#ifndef _WIN32
    ptr<QxtGlobalShortcut>
#else
    Key
#endif
    ;
    
    K keyCenter;
    K keyToggle;

    WId handle;
#ifdef _WIN32
    ptr<KeybindingWorker> keybindingWorker;
#endif
    
    struct settings {
        pbundle b;
        key_opts center, toggle;
        settings() :
            b(bundle("keyboard-shortcuts")),
            center(b, "center"),
            toggle(b, "toggle")
        {}
    } s;

    Shortcuts(WId handle) : handle(handle)
    {
        reload();
    }

    void reload()
    {
#ifndef _WIN32
        if (keyCenter)
        {
            keyCenter->setShortcut(QKeySequence::UnknownKey);
            keyCenter->setEnabled(false);
        }
        if (keyToggle)
        {
            keyToggle->setShortcut(QKeySequence::UnknownKey);
            keyToggle->setEnabled(false);
        }
#endif
        bind_keyboard_shortcut(keyCenter, s.center);
        bind_keyboard_shortcut(keyToggle, s.toggle);
#ifdef _WIN32
        keybindingWorker = nullptr;
        keybindingWorker = std::make_shared<KeybindingWorker>(keyCenter, keyToggle, handle);
        keybindingWorker->start();
#endif        
    }
private:
    void bind_keyboard_shortcut(K &key, key_opts& k);
};

class KeyboardShortcutDialog: public QWidget
{
    Q_OBJECT
public:
    KeyboardShortcutDialog();
private:
    Ui::UICKeyboardShortcutDialog ui;
    Shortcuts::settings s;
    ptr<Shortcuts> sc;
signals:
    void reload();
private slots:
	void doOK();
	void doCancel();
};
