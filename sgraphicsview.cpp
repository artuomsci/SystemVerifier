#include "sgraphicsview.h"
#include "ui_sgraphicsview.h"

//----------------------------------------------------------------------
SGraphicsView::SGraphicsView(QWidget *parent) :
    QGraphicsView   (parent),
    ui              (new Ui::SGraphicsView)
{
    ui->setupUi(this);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
}

//----------------------------------------------------------------------
SGraphicsView::~SGraphicsView()
{
    delete ui;
}

