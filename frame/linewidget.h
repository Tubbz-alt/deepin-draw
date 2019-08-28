#ifndef LINEWIDGET_H
#define LINEWIDGET_H

#include <DWidget>

#include "drawshape/globaldefine.h"


class CSideWidthWidget;
class BorderColorButton;

DWIDGET_USE_NAMESPACE

class LineWidget : public DWidget
{
    Q_OBJECT
public:
    LineWidget(QWidget *parent = nullptr);
    ~LineWidget();

signals:
    void showColorPanel(DrawStatus drawstatus, QPoint pos, bool visible = true);

private:
    CSideWidthWidget *m_sideWidthWidget;
    BorderColorButton *m_strokeButton;

private:
    void initUI();
    void initConnection();
};

#endif // LINEWIDGET_H
