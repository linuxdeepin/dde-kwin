#include <QObject>

class Utils : public QObject
{
public:
    static void passInputEvent(int wid);
    static void unPassInputEvent(int wid, QSize size);
};
