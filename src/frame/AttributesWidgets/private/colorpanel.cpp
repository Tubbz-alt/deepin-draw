/*
 * Copyright (C) 2019 ~ %YEAR% Deepin Technology Co., Ltd.
 *
 * Author:     RenRan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "colorpanel.h"

#include <DGuiApplicationHelper>

#include <QPainter>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QDebug>
#include <QRegExpValidator>

#include "utils/global.h"
#include "utils/baseutils.h"
#include "drawshape/cdrawparamsigleton.h"
#include "frame/cviewmanagement.h"
#include "frame/cgraphicsview.h"
#include "service/cmanagerattributeservice.h"

#include "ciconbutton.h"
#include "colorlabel.h"
#include "colorslider.h"
#include "pickcolorwidget.h"
#include "calphacontrolwidget.h"
#include "AttributesWidgets/ccolorpickwidget.h"

DGUI_USE_NAMESPACE

const int ORGIN_WIDTH = 314;
const int PANEL_WIDTH = 294;
const int ORIGIN_HEIGHT = 250;
const int EXPAND_HEIGHT = 475;
const int RADIUS = 8;
const QSize COLOR_BORDER_SIZE = QSize(34, 34);

ColorButton::ColorButton(const QColor &color, DWidget *parent)
    : DPushButton(parent)
    , m_color(color)
{
    setFixedSize(COLOR_BORDER_SIZE);
    setCheckable(true);

    connect(this, &ColorButton::clicked, this, [=]() {
        this->setChecked(true);
    });
}

void ColorButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setBrush(QBrush(m_color));
    QColor colorTrasparent(Qt::white);
    colorTrasparent.setAlpha(0);

    QPen pen;
    pen.setWidth(1);
    if (m_color == colorTrasparent) {
        pen.setColor(QColor("#cccccc"));
    } else {
        pen.setColor(QColor(0, 0, 0, 55));
    }
    painter.setPen(pen);
    painter.drawRoundedRect(QRect(3, 3, this->width() - 6,
                                  this->height() - 6),
                            RADIUS, RADIUS);
    if (m_color == colorTrasparent) {
        pen.setColor(QColor("#ff804d"));
        painter.setPen(pen);
        painter.drawLine(6, this->height() - 6, this->width() - 6, 6);
    }

    if (isChecked()) {
        painter.setBrush(QBrush());
        QPen borderPen;
        borderPen.setWidth(2);
        borderPen.setColor("#008eff");
        painter.setPen(borderPen);
        painter.drawRoundedRect(QRect(1, 1, this->width() - 2,
                                      this->height() - 2),
                                RADIUS, RADIUS);
    }
}

QColor ColorButton::color()
{
    return m_color;
}

ColorButton::~ColorButton()
{
}

ColorPanel::ColorPanel(DWidget *parent)
    : DWidget(parent)
    , m_expand(false)
{
    initUI();
    initConnection();
}

ColorPanel::~ColorPanel()
{
}

void ColorPanel::setTheme(int theme)
{
    int themeType = theme;
    m_colorfulBtn->setTheme(themeType);
    m_pickColWidget->setTheme(themeType);
}

void ColorPanel::initUI()
{
    DWidget *colorBtnWidget = new DWidget(this);
    colorBtnWidget->setFixedSize(ORGIN_WIDTH, ORIGIN_HEIGHT);

    m_colList = specifiedColorList();

    m_colorsButtonGroup = new QButtonGroup(this);
    m_colorsButtonGroup->setExclusive(true);

    QGridLayout *gLayout = new QGridLayout;
    gLayout->setVerticalSpacing(4);
    gLayout->setHorizontalSpacing(4);

    for (int i = 0; i < m_colList.length(); i++) {
        ColorButton *cb = new ColorButton(m_colList[i], this);
        cb->setFocusPolicy(Qt::NoFocus);
        m_cButtonList.append(cb);
        gLayout->addWidget(cb, i / 8, i % 8);
        m_colorsButtonGroup->addButton(cb, i);
    }

    m_alphaControlWidget = new CAlphaControlWidget(this);
    m_alphaControlWidget->setFocusPolicy(Qt::NoFocus);

    DWidget *colorValueWidget = new DWidget;
    colorValueWidget->setFocusPolicy(Qt::NoFocus);
    colorValueWidget->setFixedWidth(PANEL_WIDTH);
    DLabel *colLabel = new DLabel(colorValueWidget);
    QFont colLabelFont = colLabel->font();
    colLabelFont.setPixelSize(13);
    colLabel->setFixedWidth(35);
    colLabel->setText(tr("Color"));
    colLabel->setFont(colLabelFont);

    m_colLineEdit = new DLineEdit(colorValueWidget);
    m_colLineEdit->setObjectName("Color Line Edit");
    m_colLineEdit->setFixedSize(180, 36);
    m_colLineEdit->setClearButtonEnabled(false);
    m_colLineEdit->lineEdit()->setValidator(new QRegExpValidator(QRegExp("[0-9A-Fa-f]{6}"), this));
    m_colLineEdit->setText("ffffff");

    QMap<int, QMap<CIconButton::EIconButtonSattus, QString>> pictureMap;
    pictureMap[DGuiApplicationHelper::LightType][CIconButton::Normal] = QString(":/theme/light/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::LightType][CIconButton::Hover] = QString(":/theme/light/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::LightType][CIconButton::Press] = QString(":/theme/light/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::LightType][CIconButton::Active] = QString(":/theme/light/images/draw/palette_normal.svg");

    pictureMap[DGuiApplicationHelper::DarkType][CIconButton::Normal] = QString(":/theme/dark/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::DarkType][CIconButton::Hover] = QString(":/theme/dark/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::DarkType][CIconButton::Press] = QString(":/theme/dark/images/draw/palette_normal.svg");
    pictureMap[DGuiApplicationHelper::DarkType][CIconButton::Active] = QString(":/theme/dark/images/draw/palette_normal.svg");

    m_colorfulBtn = new CIconButton(pictureMap, QSize(55, 36), colorValueWidget, false);
    m_colorfulBtn->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *colorLayout = new QHBoxLayout(colorValueWidget);
    colorLayout->setMargin(0);
    colorLayout->setSpacing(0);
    colorLayout->addWidget(colLabel);
    colorLayout->addSpacing(14);
    colorLayout->addWidget(m_colLineEdit);
    colorLayout->addSpacing(10);
    colorLayout->addWidget(m_colorfulBtn);

    m_pickColWidget = new PickColorWidget(this);
    m_pickColWidget->setFocusPolicy(Qt::NoFocus);

    QVBoxLayout *vLayout = new QVBoxLayout(colorBtnWidget);
    vLayout->setSpacing(0);
    vLayout->addSpacing(10);
    vLayout->addLayout(gLayout);
    vLayout->addSpacing(10);
    vLayout->addWidget(m_alphaControlWidget, 0, Qt::AlignCenter);
    vLayout->addSpacing(10);
    vLayout->addWidget(colorValueWidget, 0, Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(colorBtnWidget);
    layout->addWidget(m_pickColWidget, 0, Qt::AlignCenter);

    if (!m_expand)
        m_pickColWidget->hide();
}

void ColorPanel::initConnection()
{
    //1.颜色按钮组
    connect(m_colorsButtonGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [=](QAbstractButton *pBtn, bool checked) {
                ColorButton *pColorBtn = qobject_cast<ColorButton *>(pBtn);
                if (checked) {
                    this->setColor(pColorBtn->color());
                }
            });

    //2.pick板颜色设置完成
    connect(m_pickColWidget, &PickColorWidget::colorChanged, this, [=](const QColor &color) {
        this->setColor(color);
    });

    //3.pick板颜色预览
    connect(m_pickColWidget, &PickColorWidget::previewedColorChanged, [=](const QColor &color) {
        this->setColor(color, true, EChangedUpdate);
    });

    //4.lineedit颜色设置
    connect(m_colLineEdit, &DLineEdit::textChanged, this, [=](const QString &colorStr) {
        //int v = colorStr.toLong(nullptr,16);
        Q_UNUSED(colorStr);
    });

    //5.设置透明度
    connect(m_alphaControlWidget, &CAlphaControlWidget::alphaChanged, this, [=](int apl, EChangedPhase phase) {
        QColor c = color();
        c.setAlpha(apl);
        this->setColor(c, true, phase);
    });

    //展开按钮
    connect(m_colorfulBtn, &CIconButton::buttonClick, this, [=] {
        if (m_expand) {
            m_pickColWidget->hide();
            //m_pickColWidget->setPickedColor(false);
            setFixedHeight(ORIGIN_HEIGHT);
            updateGeometry();
        } else {
            m_pickColWidget->show();
            //m_pickColWidget->setPickedColor(true);
            setFixedHeight(EXPAND_HEIGHT);
            updateGeometry();
        }

        if (this->parentColorWidget() != nullptr) {
            this->parentColorWidget()->setContent(this);
        }

        m_expand = !m_expand;
    });
}

CColorPickWidget *ColorPanel::parentColorWidget()
{
    return qobject_cast<CColorPickWidget *>(parent());
}

void ColorPanel::setColor(const QColor &c, bool internalChanged, EChangedPhase phase)
{
    if (!phase)
        curColor = c;

    updateColor(c);

    if (internalChanged) {
        emit colorChanged(c, phase);
    }
}

QColor ColorPanel::color()
{
    return curColor;
}

void ColorPanel::updateColor(const QColor &previewColor)
{
    QColor c = previewColor.isValid() ? previewColor : curColor;

    //1.检查当前颜色是否是颜色组的颜色值
    int id = m_colList.indexOf(curColor);
    if (id != -1) {
        m_colorsButtonGroup->blockSignals(true);
        m_colorsButtonGroup->button(id)->setChecked(true);
        m_colorsButtonGroup->blockSignals(false);
    } else {
        //m_colorsButtonGroup->
    }

    //2.pick界面刷新设置为该颜色
    m_pickColWidget->setColor(c, false);

    //3.设置当前颜色的透明度(被动设置改值)
    m_alphaControlWidget->setAlpha(c.alpha(), false);

    //4.更新颜色名字
    QString colorName = "";
    if (c.name().contains("#")) {
        colorName = c.name().split("#").last();
    }
    m_colLineEdit->blockSignals(true);
    //qDebug() << "colorName ======== " << colorName;
    m_colLineEdit->setText(colorName);
    m_colLineEdit->blockSignals(false);

    //    if (previewColor.isValid()) {
    //        emit previewedColorChanged(previewColor);
    //    }
}
