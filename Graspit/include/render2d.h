#ifndef RENDER2D_H
#define RENDER2D_H

#include <QWidget>
#include <QDialog>

#include "ivmgr.h"
#include "world.h"

class SoQtRenderArea;
class SoQtExaminerViewer;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoSeparator;
class GWS;

class Render2D : public QDialog
{
private:
    IVmgr *ivmgr;


public:
    Render2D(QWidget *parent, Hand *hand, Body *body);
    ~Render2D();
    void update();
};

#endif // RENDER2D_H
