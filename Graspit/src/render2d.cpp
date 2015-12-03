#include "render2d.h"

#include <qwidget.h>
#include <iostream>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/Qt/SoQtRenderArea.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>


#include "world.h"
#include "worldElement.h"
#include "robot.h"

Render2D::Render2D(QWidget *parent, Hand* hand, Body *body){

    this->resize(800, 600);

    ivmgr = new IVmgr(this, "Render2D");
    //ivmgr->getViewer()->setBackgroundColor(SbColor(0.0f,0.0f,0.0f));


//    Hand *hand = simuWorld->getCurrentHand();
//    if(hand == NULL){
//        std::cout << "hand is null" << std::endl;
//    }


    World *world = ivmgr->getWorld();

    Hand *newHand = new Hand(world, hand->getName());
    newHand->cloneFrom(hand);

    world->addRobot((Robot*)newHand);
    //world->setCurrentHand(newHand);
    newHand->setTran(transf::IDENTITY);


    Body *newBody = new Body(world, body->getName());
    newBody->cloneFrom(body);

    world->addBody(newBody);

    world->findAllContacts();
    world->updateGrasps();


    ivmgr->update();

}

Render2D::~Render2D(){
    if(ivmgr != NULL){
        delete ivmgr;
        ivmgr = NULL;
    }
}

void Render2D::update(){

}
