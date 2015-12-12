#ifndef RENDER2D_H
#define RENDER2D_H

#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QThread>

#include "ivmgr.h"
#include "world.h"
#include "model.h"
#include "db_manager.h"
#include "graspit_db_model.h"

#include <vector>
#include <iostream>

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(int ms){
        running = false;
        this->ms = ms;
    }
    ~RenderThread(){

    }

    void stop(){
        running = false;
    }
signals:
    void renderSignal();

private:
    bool running;
    int ms;

protected:
    void run(){
        running = true;
        while(running){
            msleep(ms);
            emit renderSignal();
        }
    }
};

class Render2D : public QDialog
{
    Q_OBJECT
private:
    IVmgr *ivmgr;
    std::vector<SbVec3f> cameraPoints;

    void sampleCameraPosition();
    RenderThread *renderThread;
    QString saveDir;
    int currentFrame;

    void saveImage(QString filename);

private slots:
    void update();

public:
    Render2D(QWidget *parent, Hand *hand, GraspableBody *body, QString saveDir);
    ~Render2D();
};

#endif // RENDER2D_H
