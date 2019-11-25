/*
 * Copyright (C) 2019 ~ %YEAR% Deepin Technology Co., Ltd.
 *
 * Author:     WangXing
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
#include "cgraphicsview.h"
#include "drawshape/cdrawparamsigleton.h"
#include "drawshape/cshapemimedata.h"
#include "drawshape/cgraphicsitem.h"
#include "drawshape/globaldefine.h"
#include "drawshape/cdrawscene.h"
#include "cundocommands.h"
#include "widgets/ctextedit.h"
#include "drawshape/cgraphicspolygonitem.h"
#include "drawshape/cgraphicspolygonalstaritem.h"
#include "drawshape/cdrawscene.h"
#include "utils/cddfmanager.h"
#include "drawshape/cgraphicsrectitem.h"
#include "drawshape/cgraphicsellipseitem.h"
#include "drawshape/cgraphicslineitem.h"
#include "drawshape/cgraphicstriangleitem.h"
#include "drawshape/cgraphicspolygonitem.h"
#include "drawshape/cgraphicspolygonalstaritem.h"
#include "drawshape/cgraphicstextitem.h"
#include "drawshape/cgraphicsmasicoitem.h"
#include "drawshape/cgraphicspenitem.h"
#include "drawshape/cpictureitem.h"

#include <DMenu>
#include <DFileDialog>

#include <QAction>
#include <QWheelEvent>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QUndoStack>
#include <QRectF>
#include <QPainter>
#include <QDesktopWidget>

CGraphicsView::CGraphicsView(DWidget *parent)
    : DGraphicsView (parent)
    , m_scale(1)
    , m_isShowContext(true)
    , m_isStopContinuousDrawing(false)
{
    setOptimizationFlags(IndirectPainting);
    m_pUndoStack = new QUndoStack(this);
    m_DDFManager = new CDDFManager (this, this);

    initContextMenu();
    initContextMenuConnection();

    //文字右键菜单初始化
    initTextContextMenu();
    initTextContextMenuConnection();

    initConnection();
}

void CGraphicsView::zoomOut()
{
    qDebug() << "m_scale=" << m_scale;
    if (1.1 * m_scale - 8 <= 0.01) {
        this->scale(1.1 * m_scale);
        emit signalSetScale(m_scale);
    } else {
        m_scale = 8;
        this->scale(m_scale);
        emit signalSetScale(m_scale);
    }
}

void CGraphicsView::zoomIn()
{
    if (0.9 * m_scale - 0.25 >= 0.01) {
        this->scale(0.9 * m_scale);
        emit signalSetScale(m_scale);

    } else {
        m_scale = 0.25;
        this->scale(m_scale);
        emit signalSetScale(m_scale);
    }
}

void CGraphicsView::scale(qreal scale)
{
    qreal multiple = scale / m_scale;
    DGraphicsView::scale(multiple, multiple);
    m_scale = scale;
    CDrawParamSigleton::GetInstance()->setScale(m_scale);
}


void CGraphicsView::wheelEvent(QWheelEvent *event)
{
    /*if (CDrawParamSigleton::GetInstance()->getCtlKeyStatus()) {
        if (event->delta() > 0) {
            zoomOut();

        } else {
            zoomIn();
        }
    }*/
}

void CGraphicsView::initContextMenu()
{
    m_contextMenu = new CMenu(this);
    m_contextMenu->setFixedWidth(182);

    m_cutAct = new QAction(tr("Cut"), this);
    m_contextMenu->addAction(m_cutAct);
    m_cutAct->setShortcut(QKeySequence::Cut);
    this->addAction(m_cutAct);

    m_copyAct = new QAction(tr("Copy"), this);
    m_contextMenu->addAction(m_copyAct);
    m_copyAct->setShortcut(QKeySequence::Copy);
    this->addAction(m_copyAct);

    m_pasteAct = new QAction(tr("Paste"), this);
    m_contextMenu->addAction(m_pasteAct);
    m_pasteAct->setShortcut(QKeySequence::Paste);
    this->addAction(m_pasteAct);

    m_selectAllAct = new QAction(tr("Select all"), this);
    m_contextMenu->addAction(m_selectAllAct);
    m_selectAllAct->setShortcut(QKeySequence::SelectAll);
    this->addAction(m_selectAllAct);

    m_contextMenu->addSeparator();

    m_deleteAct = new QAction(tr("Delete"), this);
    m_contextMenu->addAction(m_deleteAct);
    m_deleteAct->setShortcut(QKeySequence::Delete);
    this->addAction(m_deleteAct);

    //m_undoAct = m_contextMenu->addAction(tr("Undo"));
    m_undoAct = m_pUndoStack->createUndoAction(this, tr("Undo"));
    m_contextMenu->addAction(m_undoAct);
    m_undoAct->setShortcut(QKeySequence::Undo);
    this->addAction(m_undoAct);
    m_redoAct = m_pUndoStack->createRedoAction(this, tr("Redo"));
    m_contextMenu->addAction(m_redoAct);
    m_redoAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    this->addAction(m_redoAct);
    m_contextMenu->addSeparator();

    m_oneLayerUpAct = new QAction(tr("Raise Layer"), this);
    m_contextMenu->addAction(m_oneLayerUpAct);
    m_oneLayerUpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight));
    this->addAction(m_oneLayerUpAct);

    m_oneLayerDownAct = new QAction(tr("Lower Layer"), this);
    m_contextMenu->addAction(m_oneLayerDownAct);
    m_oneLayerDownAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft));
    this->addAction(m_oneLayerDownAct);

    m_bringToFrontAct = new QAction(tr("Layer to Top"), this);
    m_contextMenu->addAction(m_bringToFrontAct);
    m_bringToFrontAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketRight));
    this->addAction(m_bringToFrontAct);

    m_sendTobackAct = new QAction(tr("Layer to Bottom"), this);
    m_contextMenu->addAction(m_sendTobackAct);
    m_sendTobackAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketLeft));
    this->addAction(m_sendTobackAct);

//    m_leftAlignAct = m_contextMenu->addAction(tr("Left align"));
//    m_topAlignAct = m_contextMenu->addAction(tr("Top align"));
//    m_rightAlignAct = m_contextMenu->addAction(tr("Right align"));
//    m_centerAlignAct = m_contextMenu->addAction(tr("Center align"));


    m_cutScence = new QAction(this);
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::Key_Return));
    shortcuts.append(QKeySequence(Qt::Key_Enter));
    m_cutScence->setShortcuts(shortcuts);
    this->addAction(m_cutScence);

    m_primitiveZoomInAction = new QAction(this);
    m_primitiveZoomInAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    this->addAction(m_primitiveZoomInAction);

    m_primitiveZoomOutAction = new QAction(this);
    m_primitiveZoomOutAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    this->addAction(m_primitiveZoomOutAction);
}

void CGraphicsView::initContextMenuConnection()
{
    connect(m_cutAct, SIGNAL(triggered()), this, SLOT(slotOnCut()));
    connect(m_copyAct, SIGNAL(triggered()), this, SLOT(slotOnCopy()));
    connect(m_pasteAct, SIGNAL(triggered()), this, SLOT(slotOnPaste()));
    connect(m_selectAllAct, SIGNAL(triggered()), this, SLOT(slotOnSelectAll()));
    connect(m_deleteAct, SIGNAL(triggered()), this, SLOT(slotOnDelete()));
    connect(m_bringToFrontAct, SIGNAL(triggered()), this, SLOT(slotBringToFront()));
    connect(m_sendTobackAct, SIGNAL(triggered()), this, SLOT(slotSendTobackAct()));
    connect(m_oneLayerUpAct, SIGNAL(triggered()), this, SLOT(slotOneLayerUp()));
    connect(m_oneLayerDownAct, SIGNAL(triggered()), this, SLOT(slotOneLayerDown()));
    connect(m_cutScence, SIGNAL(triggered()), this, SLOT(slotDoCutScene()));

    connect(m_primitiveZoomInAction, SIGNAL(triggered()), this, SLOT(slotPrimitiveZoomIn()));

    connect(m_primitiveZoomOutAction, SIGNAL(triggered()), this, SLOT(slotPrimitiveZoomOut()));
    //右键菜单隐藏时更新菜单选项层位操作可用，方便快捷键使用
    connect(m_contextMenu, &DMenu::aboutToHide, this, [ = ]() {
        m_bringToFrontAct->setEnabled(true);
        m_sendTobackAct->setEnabled(true);
        m_oneLayerUpAct->setEnabled(true);
        m_oneLayerDownAct->setEnabled(true);
    });
}

void CGraphicsView::initTextContextMenu()
{
    m_textMenu = new CMenu(this);
    m_textMenu->setFixedWidth(182);

    m_textCutAction = new QAction(tr("Cut"));
    m_textCopyAction = new QAction(tr("Copy"));
    m_textPasteAction = new QAction(tr("Paste"));
    m_textSelectAllAction = new QAction(tr("Select All"));

    QAction *deleteAct = new QAction(tr("Delete"));
    deleteAct->setEnabled(false);
    QAction *undoAct = new QAction(tr("Undo"));
    undoAct->setEnabled(false);

    QAction *fakeRaiseLayerAct = new QAction(tr("Raise Layer"));
    fakeRaiseLayerAct->setEnabled(false);
    QAction *fakeLowerLayerAct = new QAction(tr("Lower Layer"));
    fakeLowerLayerAct->setEnabled(false);
    QAction *fakeLayerToTopAct = new QAction(tr("Layer to Top"));
    fakeLayerToTopAct->setEnabled(false);
    QAction *fakeLayerToBottomAct = new QAction(tr("Layer to Bottom"));
    fakeLayerToBottomAct->setEnabled(false);

    m_textLeftAlignAct = new QAction(tr("Left Alignment"));
    //m_textTopAlignAct = new QAction(tr("Top Alignment"));
    m_textRightAlignAct = new QAction(tr("Right Alignment" ));
    m_textCenterAlignAct = new QAction(tr("Center Alignment"));

    m_textMenu->addAction(m_textCutAction);
    m_textMenu->addAction(m_textCopyAction);
    m_textMenu->addAction(m_textPasteAction);
    m_textMenu->addAction(m_textSelectAllAction);
    m_textMenu->addSeparator();

    m_textMenu->addAction(deleteAct);
    m_textMenu->addAction(undoAct);
    m_textMenu->addSeparator();

    m_textMenu->addAction(fakeRaiseLayerAct);
    m_textMenu->addAction(fakeLowerLayerAct);
    m_textMenu->addAction(fakeLayerToTopAct);
    m_textMenu->addAction(fakeLayerToBottomAct);

    m_textMenu->addAction(m_textLeftAlignAct);
    //m_textMenu->addAction(m_textTopAlignAct);
    m_textMenu->addAction(m_textRightAlignAct);
    m_textMenu->addAction(m_textCenterAlignAct);
}

void CGraphicsView::initTextContextMenuConnection()
{
    connect(m_textCutAction, SIGNAL(triggered()), this, SLOT(slotOnTextCut()));
    connect(m_textCopyAction, SIGNAL(triggered()), this, SLOT(slotOnTextCopy()));
    connect(m_textPasteAction, SIGNAL(triggered()), this, SLOT(slotOnTextPaste()));
    connect(m_textSelectAllAction, SIGNAL(triggered()), this, SLOT(slotOnTextSelectAll()));


    connect(m_textLeftAlignAct, SIGNAL(triggered()), this, SLOT(slotOnTextLeftAlignment()));
    //connect(m_textTopAlignAct, SIGNAL(triggered()), this, SLOT(slotOnTextTopAlignment()));
    connect(m_textRightAlignAct, SIGNAL(triggered()), this, SLOT(slotOnTextRightAlignment()));
    connect(m_textCenterAlignAct, SIGNAL(triggered()), this, SLOT(slotOnTextCenterAlignment()));
}

void CGraphicsView::initConnection()
{
    connect(m_DDFManager, SIGNAL(signalClearSceneBeforLoadDDF()), this, SLOT(clearScene()));
    connect(m_DDFManager, SIGNAL(signalStartLoadDDF(QRectF)), this, SLOT(slotStartLoadDDF(QRectF)));
    connect(m_DDFManager, SIGNAL(signalAddItem(QGraphicsItem *)), this, SLOT(slotAddItemFromDDF(QGraphicsItem *)));
    connect(m_DDFManager, SIGNAL(signalContinueDoOtherThing()), this, SIGNAL(signalTransmitContinueDoOtherThing()));
    connect(m_DDFManager, SIGNAL(singalEndLoadDDF()), this, SIGNAL(singalTransmitEndLoadDDF()));
}



void CGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    QPointF pos = this->mapToScene(event->pos());
    QRectF rect = this->scene()->sceneRect();

    if (!rect.contains(pos)) {
        return;
    }

    if (!m_isShowContext) {
        return;
    }

    //获取右键菜单的显示位置，左边工具栏宽度为162，顶端参数配置栏高度为50，右键菜单三种分别为224\350\480.
    QPoint menuPos;
    int rx;
    int ry;
    //qDebug() << cursor().pos() << m_contextMenu->rect()  << this->rect() << endl;
    if (cursor().pos().rx() - 50 > this->width() - 182) {
        rx = this->width() - 182 + 50;
    } else {
        rx = cursor().pos().rx();
    }
    int temp;


    if (!scene()->selectedItems().isEmpty()) {

        m_copyAct->setEnabled(true);
        m_cutAct->setEnabled(true);
        m_deleteAct->setEnabled(true);
        m_bringToFrontAct->setVisible(true);
        m_sendTobackAct->setVisible(true);
        m_oneLayerUpAct->setVisible(true);
        m_oneLayerDownAct->setVisible(true);
        m_visible = true;
        //如果是文字图元则显示其自己的右键菜单
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsItem *tmpitem = static_cast<CGraphicsItem *>(item);
        if (TextType == item->type() &&  static_cast<CGraphicsTextItem *>(tmpitem)->isEditable()) {
            temp = 480;
            if (cursor().pos().ry() - 50 > this->height() - temp) {
                ry = this->height() - temp + 45;
            } else {
                ry = cursor().pos().ry();
            }
            menuPos = QPoint(rx, ry);
            m_textMenu->move(menuPos);
            m_textMenu->show();
            return;
        }
    } else {
        m_copyAct->setEnabled(false);
        m_cutAct->setEnabled(false);
        m_deleteAct->setEnabled(false);

        m_bringToFrontAct->setVisible(false);
        m_sendTobackAct->setVisible(false);
        m_oneLayerUpAct->setVisible(false);
        m_oneLayerDownAct->setVisible(false);
        m_visible = false;
    }
    //判定是长右键菜单还是短右键菜单;
    if (m_visible) {
        temp = 350;
    } else {
        temp = 224;
    }

    if (cursor().pos().ry() - 50 > this->height() - temp) {
        ry = this->height() - temp + 45;
    } else {
        ry = cursor().pos().ry();
    }
    menuPos = QPoint(rx, ry);

    if (scene()->items().isEmpty()) {
        m_selectAllAct->setEnabled(false);
    } else {
        m_selectAllAct->setEnabled(true);
    }


    bool layerUp = canLayerUp();
    m_oneLayerUpAct->setEnabled(layerUp);
    m_bringToFrontAct->setEnabled(layerUp);

    bool layerDown = canLayerDown();
    m_oneLayerDownAct->setEnabled(layerDown);
    m_sendTobackAct->setEnabled(layerDown);

    m_contextMenu->move(menuPos);
    m_undoAct->setEnabled(m_pUndoStack->canUndo());
    m_redoAct->setEnabled(m_pUndoStack->canRedo());
    m_pasteAct->setEnabled(QApplication::clipboard()->ownsClipboard());
    m_contextMenu->show();
}

void CGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
}

void CGraphicsView::paintEvent(QPaintEvent *event)
{
    DGraphicsView::paintEvent(event);
}

void CGraphicsView::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[])
{
    QRectF rect = scene()->sceneRect();
    painter->setClipping(true);
    painter->setClipRect(rect);
    DGraphicsView::drawItems(painter, numItems, items, options);
}

void CGraphicsView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    qApp->setOverrideCursor(Qt::ArrowCursor);
}

//QPainter *CGraphicsView::sharedPainter() const
//{
//    return m_painter;
//}

void CGraphicsView::itemMoved(QGraphicsItem *item, const QPointF &oldPosition)
{
    if ( item != nullptr) {
        QUndoCommand *moveCommand = new CMoveShapeCommand(item, oldPosition);
        m_pUndoStack->push(moveCommand);
    } else {
        QUndoCommand *moveCommand = new CMoveShapeCommand(this->scene(), oldPosition);
        m_pUndoStack->push(moveCommand);
    }
}

void CGraphicsView::itemAdded(QGraphicsItem *item)
{
    QUndoCommand *addCommand = new CAddShapeCommand(item, item->scene());
    m_pUndoStack->push(addCommand);
}

void CGraphicsView::itemRotate(QGraphicsItem *item, const qreal oldAngle)
{
    QUndoCommand *rotateCommand = new CRotateShapeCommand(item, oldAngle);
    m_pUndoStack->push(rotateCommand);
}

void CGraphicsView::itemResize(CGraphicsItem *item, CSizeHandleRect::EDirection handle, QPointF beginPos, QPointF endPos, bool bShiftPress, bool bALtPress)
{
    QUndoCommand *resizeCommand = new CResizeShapeCommand(item, handle, beginPos,  endPos, bShiftPress, bALtPress);
    m_pUndoStack->push(resizeCommand);
}

void CGraphicsView::itemPropertyChange(CGraphicsItem *item, QPen pen, QBrush brush, bool bPenChange, bool bBrushChange)
{
    QUndoCommand *setPropertyCommand = new CSetPropertyCommand(item, pen, brush, bPenChange, bBrushChange);
    m_pUndoStack->push(setPropertyCommand);
}

void CGraphicsView::itemPolygonPointChange(CGraphicsPolygonItem *item, int oldNum)
{
    QUndoCommand *command = new CSetPolygonAttributeCommand(item, oldNum);
    m_pUndoStack->push(command);
}

void CGraphicsView::itemPenTypeChange(CGraphicsPenItem *item, int oldType)
{
    QUndoCommand *command = new CSetPenAttributeCommand(item, oldType);
    m_pUndoStack->push(command);
}

void CGraphicsView::itemPolygonalStarPointChange(CGraphicsPolygonalStarItem *item, int oldNum, int oldRadius)
{
    QUndoCommand *command = new CSetPolygonStarAttributeCommand(item, oldNum, oldRadius);
    m_pUndoStack->push(command);
}

void CGraphicsView::slotStopContinuousDrawing()
{
    m_isStopContinuousDrawing = true;
}

void CGraphicsView::slotStartLoadDDF(QRectF rect)
{
    scene()->setSceneRect(rect);
}

void CGraphicsView::slotAddItemFromDDF(QGraphicsItem *item)
{
    scene()->addItem(item);
    itemAdded(item);
    if (item->type() == BlurType) {
        static_cast<CGraphicsMasicoItem *>(item)->setPixmap();
    }

}


void CGraphicsView::slotOnCut()
{
    QList<QGraphicsItem *> itemList = scene()->selectedItems();
    if (itemList.isEmpty()) {
        return;
    }

    CShapeMimeData *data = new CShapeMimeData(itemList);
    QApplication::clipboard()->setMimeData(data);

    QUndoCommand *deleteCommand = new CRemoveShapeCommand(this->scene());
    m_pUndoStack->push(deleteCommand);

    if (!m_pasteAct->isEnabled()) {
        m_pasteAct->setEnabled(true);
    }
}

void CGraphicsView::slotOnCopy()
{
    if (scene()->selectedItems().isEmpty()) {
        return;
    }

    CShapeMimeData *data = new CShapeMimeData( scene()->selectedItems() );
    QApplication::clipboard()->setMimeData(data);

    if (!m_pasteAct->isEnabled()) {
        m_pasteAct->setEnabled(true);
    }
}

void CGraphicsView::slotOnPaste()
{
    QMimeData *mp = const_cast<QMimeData *>(QApplication::clipboard()->mimeData());
    CShapeMimeData *data = dynamic_cast< CShapeMimeData *>( mp );
    if ( data ) {
        scene()->clearSelection();
        foreach (CGraphicsItem *item, data->itemList() ) {
            CGraphicsItem *copy = nullptr;

            switch (item->type()) {
            case RectType:
                copy = new CGraphicsRectItem();
                break;
            case EllipseType:
                copy = new CGraphicsEllipseItem();
                break;
            case TriangleType:
                copy = new CGraphicsTriangleItem();
                break;
            case PolygonalStarType:
                copy = new CGraphicsPolygonalStarItem();
                break;

            case PolygonType:
                copy = new CGraphicsPolygonItem();
                break;
            case LineType:
                copy = new CGraphicsLineItem();
                break;

            case PenType:
                copy = new CGraphicsPenItem();
                break;
            case TextType:
                copy = new CGraphicsTextItem();
                break;

            case PictureType:
                copy = new CPictureItem();
                break;
            case BlurType:
                copy = new CGraphicsMasicoItem();
                break;

            }

            item->duplicate(copy);
            if ( copy ) {
                copy->setSelected(true);
                copy->moveBy(10, 10);

                QUndoCommand *addCommand = new CAddShapeCommand(copy, this->scene());
                m_pUndoStack->push(addCommand);
            }
        }
    }
}


void CGraphicsView::slotOnSelectAll()
{
    scene()->clearSelection();
    foreach (QGraphicsItem *item, scene()->items()) {
        if (item->type() > QGraphicsItem::UserType) {
            item->setSelected(true);
        }
    }
}

void CGraphicsView::slotOnDelete()
{
    if (scene()->selectedItems().isEmpty()) {
        return;
    }

    QUndoCommand *deleteCommand = new CRemoveShapeCommand(this->scene());
    m_pUndoStack->push(deleteCommand);
}

void CGraphicsView::slotOneLayerUp()
{
    if (!canLayerUp()) {
        return;
    }

    QList<QGraphicsItem *> selectedList = scene()->selectedItems();


    QGraphicsItem *selectedItem = selectedList.first();

    QUndoCommand *command = new COneLayerUpCommand(selectedItem, this->scene());
    m_pUndoStack->push(command);
}

void CGraphicsView::slotOneLayerDown()
{
    if (!canLayerDown()) {
        return;
    }

    QList<QGraphicsItem *> selectedList = scene()->selectedItems();



    QGraphicsItem *selectedItem = selectedList.first();

    QUndoCommand *command = new COneLayerDownCommand(selectedItem, this->scene());
    m_pUndoStack->push(command);

}

void CGraphicsView::slotBringToFront()
{
    if (!canLayerUp()) {
        return;
    }

    QList<QGraphicsItem *> selectedList = scene()->selectedItems();

    QGraphicsItem *selectedItem = selectedList.first();

    QUndoCommand *command = new CBringToFrontCommand(selectedItem, this->scene());
    m_pUndoStack->push(command);
}

void CGraphicsView::slotSendTobackAct()
{
    if (!canLayerDown()) {
        return;
    }
    QList<QGraphicsItem *> selectedList = scene()->selectedItems();

    QGraphicsItem *selectedItem = selectedList.first();

    QUndoCommand *command = new CSendToBackCommand(selectedItem, this->scene());
    m_pUndoStack->push(command);

}

void CGraphicsView::slotQuitCutMode()
{
    static_cast<CDrawScene *>(scene())->quitCutMode();
}

void CGraphicsView::slotDoCutScene()
{
    static_cast<CDrawScene *>(scene())->doCutScene();
    //    qDebug() << "***************DoCut";
}

void CGraphicsView::slotRestContextMenuAfterQuitCut()
{
    setContextMenuAndActionEnable(true);
}

void CGraphicsView::slotPrimitiveZoomIn()
{
    if (m_scale + 0.25 <= 8) {
        this->scale(m_scale + 0.25);
        emit signalSetScale(m_scale);
    } else {
        m_scale = 8;
        this->scale(m_scale);
        emit signalSetScale(m_scale);
    }
}

void CGraphicsView::slotPrimitiveZoomOut()
{
    if (m_scale - 0.25 >= 0.01) {
        this->scale(m_scale - 0.25);
        emit signalSetScale(m_scale);

    } else {
        m_scale = 0.25;
        this->scale(m_scale);
        emit signalSetScale(m_scale);
    }
}

void CGraphicsView::slotOnTextCut()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doCut();
        }
    }
}

void CGraphicsView::slotOnTextCopy()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doCopy();
        }
    }
}

void CGraphicsView::slotOnTextPaste()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doPaste();
        }
    }
}

void CGraphicsView::slotOnTextSelectAll()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doSelectAll();
        }
    }
}

void CGraphicsView::slotOnTextTopAlignment()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doTopAlignment();
        }
    }
}

void CGraphicsView::slotOnTextRightAlignment()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doRightAlignment();
        }
    }
}

void CGraphicsView::slotOnTextLeftAlignment()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doLeftAlignment();
        }
    }
}

void CGraphicsView::slotOnTextCenterAlignment()
{
    if (!scene()->selectedItems().isEmpty()) {
        QGraphicsItem *item =  scene()->selectedItems().first();
        CGraphicsTextItem *tmpitem = static_cast<CGraphicsTextItem *>(item);
        if (TextType == item->type() &&  tmpitem->isEditable()) {
            tmpitem->doCenterAlignment();
        }
    }
}

void CGraphicsView::clearScene()
{
    ///清除场景选中
    scene()->clearSelection();
    //清空撤销栈
    m_pUndoStack->clear();
    //清空场景
    scene()->clear();
}

void CGraphicsView::doSaveDDF()
{
    QString ddfPath = CDrawParamSigleton::GetInstance()->getDdfSavePath();
    if (ddfPath.isEmpty() || ddfPath == "") {
        showSaveDDFDialog(true);
    } else {
        m_DDFManager->saveToDDF(ddfPath, scene());
    }
}

void CGraphicsView::showSaveDDFDialog(bool type)
{
    DFileDialog dialog(this);
    if (type) {
        dialog.setWindowTitle(tr("保存文件"));
    } else {
        dialog.setWindowTitle(tr("另存为"));
    }//设置文件保存对话框的标题
    dialog.setAcceptMode(QFileDialog::AcceptSave);//设置文件对话框为保存模式
    dialog.setOptions(QFileDialog::DontResolveSymlinks);//只显示文件夹
    dialog.setViewMode(DFileDialog::List);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.selectFile(tr("画图.DDF"));//设置默认的文件名
    QStringList nameFilters;
    nameFilters << "*.DDF";
    dialog.setNameFilters(nameFilters);//设置文件类型过滤器
    if (dialog.exec()) {
        QString path = dialog.selectedFiles().first();
        if (!path.isEmpty()) {
            m_DDFManager->saveToDDF(path, scene());
        }
    }
}

void CGraphicsView::doImport()
{
    DFileDialog dialog(this);
    dialog.setWindowTitle(tr("打开文件"));//设置文件保存对话框的标题
    dialog.setAcceptMode(QFileDialog::AcceptOpen);//设置文件对话框为保存模式
    dialog.setViewMode(DFileDialog::List);
    dialog.setFileMode(DFileDialog::ExistingFile);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    //dialog.set
    QStringList nameFilters;
    nameFilters << "*.DDF *.png *.jpg *.bmp *.tif";
    dialog.setNameFilters(nameFilters);//设置文件类型过滤器
    if (dialog.exec()) {
        QString path = dialog.selectedFiles().first();
        if (!path.isEmpty() && QFileInfo(path).suffix() == "DDF") {
            m_DDFManager->loadDDF(path);
        } else {
            emit signalImportPicture(path);
        }
    }
}

void CGraphicsView::importData(const QString &path)
{
    if (!path.isEmpty() && QFileInfo(path).suffix() == "DDF") {
        m_DDFManager->loadDDF(path, true);
    } else {
        emit signalImportPicture(path);
    }
}

void CGraphicsView::disableCutShortcut(bool isFocus)
{
    m_cutScence->setEnabled(!isFocus);
//    qDebug() << "m_cutScence->isEnabled()=" << m_cutScence->isEnabled();
}


void CGraphicsView::setContextMenuAndActionEnable(bool enable)
{
    m_isShowContext = enable;
    m_cutAct->setEnabled(enable);
    m_copyAct->setEnabled(enable);
    m_pasteAct->setEnabled(enable);
    m_deleteAct->setEnabled(enable);
    m_selectAllAct->setEnabled(enable);
    m_undoAct->setEnabled(enable);
    m_redoAct->setEnabled(enable);
    m_oneLayerUpAct->setEnabled(enable);
    m_oneLayerDownAct->setEnabled(enable);
    m_bringToFrontAct->setEnabled(enable);
    m_sendTobackAct->setEnabled(enable);
}

bool CGraphicsView::canLayerUp()
{
    QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
    QList<QGraphicsItem *> allItems = scene()->items();

    int trueItemCount = 0;

    foreach (QGraphicsItem *item, allItems) {
        if (item->type() > QGraphicsItem::UserType) {
            trueItemCount ++;
            if (trueItemCount > 1) {
                break;
            }
        }
    }

    if (selectedItems.count() > 1 || selectedItems.count() <= 0 || trueItemCount <= 1) {
        return false;
    }
    QGraphicsItem *selectItem = selectedItems.first();



    int index = allItems.indexOf(selectItem);

    if (index == selectItem->childItems().count()) {
        return false;
    }

    return true;
}

bool CGraphicsView::canLayerDown()
{
    QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
    QList<QGraphicsItem *> allItems = scene()->items(Qt::AscendingOrder);

    int trueItemCount = 0;

    foreach (QGraphicsItem *item, allItems) {
        if (item->type() > QGraphicsItem::UserType) {
            trueItemCount ++;
            if (trueItemCount > 1) {
                break;
            }
        }
    }

    if (selectedItems.count() > 1 || selectedItems.count() <= 0 || trueItemCount <= 1) {
        return false;
    }

    QGraphicsItem *item = selectedItems.first();



    int index = allItems.indexOf(item);

    if (index == 0) {
        return false;
    }

    return true;
}

