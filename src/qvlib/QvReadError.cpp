#include <stdarg.h>
#include <string.h>
#include <QvString.h>
#include <QvInput.h>
#include <QvReadError.h>

static char errorText[512];
static char warnText[1024];

void
QvReadError::post(const QvInput *in, const char *formatString ...)
{
    char	buf[10000];
    va_list	ap;

    va_start(ap, formatString);
    vsprintf(buf, formatString, ap);
    va_end(ap);

    QvString locstr;
    in->getLocationString(locstr);
    fprintf(stderr, "VRML read error: %s\n%s\n", buf, locstr.getString());
    if (!errorText[0])
        sprintf(errorText, "%.400s\n%.100s", buf, locstr.getString());
}

int QvReadError::lenient = 1;

void
QvReadError::warn(const QvInput *in, const char *formatString ...)
{
    char	buf[512];
    va_list	ap;

    va_start(ap, formatString);
    vsprintf(buf, formatString, ap);
    va_end(ap);

    QvString locstr;
    in->getLocationString(locstr);
    fprintf(stderr, "VRML read warning: %s\n%s\n", buf, locstr.getString());
    if (strstr(warnText, buf) == NULL &&
        strlen(warnText) + strlen(buf) + 2 < sizeof(warnText)) {
        strcat(warnText, buf);
        strcat(warnText, "\n");
    }
}

void
QvReadError::clear()
{
    errorText[0] = '\0';
    warnText[0] = '\0';
}

const char *
QvReadError::getError()
{
    return errorText;
}

const char *
QvReadError::getWarnings()
{
    return warnText;
}
