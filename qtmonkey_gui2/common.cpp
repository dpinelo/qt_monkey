#include "common.h"

QStringList splitCommandLine(const QString &cmdLine)
{
    QStringList list;
    QString arg;
    bool escape = false;
    enum { Idle, Arg, QuotedArg } state = Idle;
    for (QChar const c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }
        switch (state) {
        case Idle:
            if (!escape && c == '"')
                state = QuotedArg;
            else if (escape || !c.isSpace()) {
                arg += c;
                state = Arg;
            }
            break;
        case Arg:
            if (!escape && c == '"') {
                state = QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
            } else {
                list << arg;
                arg.clear();
                state = Idle;
            }
            break;
        case QuotedArg:
            if (!escape && c == '"')
                state = arg.isEmpty() ? Idle : Arg;
            else
                arg += c;
            break;
        }
        escape = false;
    }
    if (!arg.isEmpty())
        list << arg;
    return list;
}
