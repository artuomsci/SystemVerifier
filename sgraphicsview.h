#ifndef SGRAPHICSVIEW_H
#define SGRAPHICSVIEW_H

#include <QWidget>
#include <QGraphicsView>

namespace Ui {
class SGraphicsView;
}

class SGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SGraphicsView(QWidget *parent = 0);
    ~SGraphicsView();

private:
    Ui::SGraphicsView *ui;
};

#endif // SGRAPHICSVIEW_H
