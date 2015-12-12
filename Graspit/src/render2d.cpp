#include "render2d.h"

#include <qwidget.h>
#include <iostream>
#include <qthread.h>
#include <QDir>


#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoBoxHighlightRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/draggers/SoHandleBoxDragger.h>
#include <Inventor/draggers/SoRotateDiscDragger.h>
#include <Inventor/draggers/SoCenterballDragger.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/nodekits/SoWrapperKit.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoBlinker.h>
#include <Inventor/nodes/SoColorIndex.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoFile.h>
#include <Inventor/sensors/SoIdleSensor.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/Qt/SoQt.h>


#include "world.h"
#include "worldElement.h"
#include "robot.h"

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};



Render2D::Render2D(QWidget *parent, Hand* hand, GraspableBody *body, QString saveDir) :QDialog(parent){

    this->resize(1280, 720);

    this->setWindowTitle("Render2D");
    this->saveDir = saveDir;
    ivmgr = new IVmgr(this, "Render2D");

    World *world = ivmgr->getWorld();
    StereoViewer *viewer = ivmgr->getViewer();
    viewer->setAnimationEnabled(false);

    viewer->setBackgroundColor(SbColor(0.0f,1.0f,0.0f));


    Hand *newHand = new Hand(world, hand->getName());
    newHand->cloneFrom(hand);
    world->addRobot((Robot*)newHand);
    newHand->setTran(hand->getTran());

    Body *newBody = new Body(world, body->getName());
    newBody->cloneFrom(body);
    world->addBody(newBody);
    newBody->setTran(body->getTran());

    SoSeparator *contactIndicator = new SoSeparator();
    SoMaterial *mat = new SoMaterial;
    mat->diffuseColor = SbColor(1.0f,0,0.0f);
    mat->emissiveColor = SbColor(1.0f,0,0.0f);
    mat->ambientColor = SbColor(1.0,0.0f,0.0f);
    contactIndicator->addChild(mat);

    world->getIVRoot()->addChild(contactIndicator);

    SoSeparator *root = newBody->getIVGeomRoot();
    for(int i= 0; i < root->getNumChildren(); i++){
        SoNode *node = root->getChild(i);
        std::string name(node->getTypeId().getName().getString());
        if(name == "Material"){
            SoMaterial *m = (SoMaterial*)node;
            m->diffuseColor = SbColor(1.0f,0,0.0f);
            m->emissiveColor = SbColor(1.0f,0,0.0f);
            m->ambientColor = SbColor(1.0f,0.0f,0.0f);
            m->diffuseColor.setIgnored(false);
            m->emissiveColor.setIgnored(false);
            m->ambientColor.setIgnored(false);
            //m->transparency = 1.0f;
        }
    }

    std::list<Contact *> contacts = body->getContacts();
    std::list<Contact *>::iterator it = contacts.begin();
    while(it != contacts.end()){
        Contact *c = *it;
        SoSphere *sphere = new SoSphere();
        sphere->radius = 5;
        SoSeparator *sep = new SoSeparator();
        SoTransform *tran = new SoTransform();
        position p = c->getPosition();
        tran->translation.setValue(p.x(), p.y(), p.z());
        sep->addChild(tran);
        sep->addChild(sphere);
        contactIndicator->addChild(sep);
        ++it;
    }

    viewer->viewAll();

    //world->removeElementFromSceneGraph(newHand);

    sampleCameraPosition();

    currentFrame = 0;

    renderThread= new RenderThread(300);
    connect(renderThread, SIGNAL(renderSignal()), this, SLOT(update()));
    renderThread->start();

}

Render2D::~Render2D(){

    renderThread->stop();
    renderThread->wait();

    if(ivmgr != NULL){
        delete ivmgr;
        ivmgr = NULL;
    }
}

void Render2D::update(){

    if(currentFrame >= cameraPoints.size()){
        renderThread->stop();
        return;
    }

    std::cout << "update: " << currentFrame << std::endl;

    StereoViewer *viewer = ivmgr->getViewer();
    SoCamera *pCamera = viewer->getCamera();
    SbMatrix mx;
    mx = pCamera->orientation.getValue();
    SbVec3f viewVec(-mx[2][0], -mx[2][1], -mx[2][2]);
    SbVec3f camPos  = pCamera->position.getValue();
    float   focDist = pCamera->focalDistance.getValue();
    SbVec3f focalPt = camPos + (focDist * viewVec);

    std::cout << focalPt[0] << "," << focalPt[1] << "," << focalPt[2] << std::endl;
    std::cout << focDist << std::endl;

    SbVec3f xyz = cameraPoints[currentFrame++];

    pCamera->position = focalPt + (focDist * xyz);
    pCamera->pointAt(focalPt);

    //viewer->viewAll();
    //ivmgr->update();


    QString name = QString("%1.png").arg(currentFrame);
    QString filename = QDir(saveDir).filePath(name);

    std::cout << "saving: " << filename.toStdString() << std::endl;

    saveImage(filename);
}


void
Render2D::saveImage(QString filename)
{
  SoQtRenderArea *renderArea;
  SoNode *sg;                // scene graph
  const SbColor white(1, 1, 1);
  const SbColor black(0,0,0);
  const SbColor green(0,1,0);
  SoGLRenderAction *glRend;
  SoOffscreenRenderer *myRenderer;

  StereoViewer *myViewer = ivmgr->getViewer();
  renderArea = myViewer;
  sg = myViewer->getSceneGraph();

  glRend = new SoGLRenderAction(renderArea->getViewportRegion());
  glRend->setSmoothing(TRUE);
  glRend->setNumPasses(5);
  glRend->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);

  myRenderer = new SoOffscreenRenderer(glRend);
  myRenderer->setBackgroundColor(green);

  int numtypes = myRenderer->getNumWriteFiletypes();
  //SbList<SbName> extList;
  SbPList extList;
  SbString fullname;
  SbString desc;
  for (int i=0;i<numtypes;i++) {
    myRenderer ->getWriteFiletypeInfo(i,extList,fullname,desc);
  }

  SoSeparator *renderRoot = new SoSeparator;
  renderRoot->ref();
  renderRoot->addChild(myViewer->getCamera());
  SoTransformSeparator *lightSep = new SoTransformSeparator;
  SoRotation *lightDir = new SoRotation;
  lightDir->rotation.connectFrom(&myViewer->getCamera()->orientation);
  lightSep->addChild(lightDir);
  lightSep->addChild(myViewer->getHeadlight());

  renderRoot->addChild(lightSep);
  renderRoot->addChild(sg);

  myRenderer->render(renderRoot);

  std::cout << filename.toStdString() << std::endl;

  SbBool result;
  result = myRenderer->writeToFile(SbString(filename.latin1()),
                   SbName(filename.section('.',-1)));

  if(!result){
      std::cout << "saveImage failed!" << std::endl;
  }

  renderRoot->unref();
  delete myRenderer;
}

void Render2D::sampleCameraPosition(){

    /*
    https://people.sc.fsu.edu/~jburkardt/cpp_src/sphere_fibonacci_grid/sphere_fibonacci_grid.cpp
  */

    int ng = 128;
    double cphi;
    double i_r8;
    int j;
    double ng_r8;
    double r8_phi;
    const double r8_pi = 3.141592653589793;
    double sphi;
    double theta;

    r8_phi = ( 1.0 + sqrt ( 5.0 ) ) / 2.0;
    ng_r8 = ( double ) ( ng );

    for ( j = 0; j < ng; j++ )
    {
      i_r8 = ( double ) ( - ng + 1 + 2 * j );
      theta = 2.0 * r8_pi * i_r8 / r8_phi;
      sphi = i_r8 / ng_r8;
      cphi = sqrt ( ( ng_r8 + i_r8 ) * ( ng_r8 - i_r8 ) ) / ng_r8;
      double z = cphi * sin ( theta );
      double x = cphi * cos ( theta );
      double y = sphi;

      std::cout << x << "," << y << "," << z << ":" << x*x+y*y+z*z << std::endl;

      cameraPoints.push_back(SbVec3f((float)x, (float)y, (float)z));
    }

}
