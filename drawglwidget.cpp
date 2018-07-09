#include "drawglwidget.h"

#define USEALPHA 1

GLfloat lightpos[]={0.0, 0.0, 50.0, 1.0};
static const GLfloat lightamb[] = { 0.2, 0.2, 0.2, 1.0 };
static const GLfloat lightdif[] = { 1.0, 1.0, 1.0, 1.0 };
static const GLfloat lightspe[] = { 1.0, 1.0, 1.0, 1.0 };

GLdouble camerazoom=15.0;
GLdouble cameravec[]={0.0, 0.0, 0.0};

const GLdouble drawGLWidget::genfunc[][4] = {
  { 1.0, 0.0, 0.0, 0.0 },
  { 0.0, 1.0, 0.0, 0.0 },
  { 0.0, 0.0, 1.0, 0.0 },
  { 0.0, 0.0, 0.0, 1.0 },
  };

const int imageWidth = 1024;
const int imageHeight = 1024;
const int cannyImageWidth = 128;
const int cannyImageHeight = 128;
const int wandHWidth = 240;
const int wandHHeight = 240;


//queで探査機の位置を記録していく　最大10個?
extern void plinfo_(SpiceInt*, SpiceInt*, SpiceInt[3], SpiceDouble[3][3], SpiceDouble[3], SpiceDouble[3], SpiceDouble*);

drawGLWidget::drawGLWidget(QWidget *parent) :
    QGLWidget(parent), timer(new QBasicTimer)
{
    targetFrame="HAYABUSA_HP";
    targetName="ITOKAWA";
    instName="HAYABUSA_AMICA";
    instId=-130102;
    strcpy(utc, "0000-00-00T00:00:00");
    start_et=end_et=et=0.0;
    preET=-1;
    target_pos[0]=target_pos[1]=target_pos[2]=0.0;
    sc_pos[0]=sc_pos[1]=sc_pos[2]=0.0;
    earth_pos[0]=earth_pos[1]=earth_pos[2]=0.0;
    sun_pos[0]=sun_pos[1]=sun_pos[2]=0.0;
    phase=incidence=emission=0.0;
    lt_earth=lt_target=0.0;
    bsight[0]=bsight[1]=bsight[2]=0.0;
    up_inst[0]=up_inst[1]=up_inst[2]=0.0;
    rotateSC[0]=rotateSC[1]=rotateSC[2]=0.0;
    observerRotate[0]=observerRotate[1]=observerRotate[2]=0.0;
    targetRotate[0]=targetRotate[1]=targetRotate[2]=0.0;
    observerTranslate[0]=observerTranslate[1]=observerTranslate[2]=0.0;
    targetTranslate[0]=targetTranslate[1]=targetTranslate[2]=0.0;
    SCPosTranslate[0]=SCPosTranslate[1]=SCPosTranslate[2]=0.0;
    geometryTranslate[0]=geometryTranslate[1]=geometryTranslate[2]=0.0;
    geometryRotate[0]=geometryRotate[1]=geometryRotate[2]=0.0;
    glViewMode="fovMode";
    animation=initializedFootprint=isOnceRestore=isRecordSubPointProjection=GL_FALSE;
    harmonicsMode="Get Data from SPICE Kernels";
    HARMONICSModes.append("Get Data from SPICE Kernels");
    HARMONICSModes.append("Target Pointing");
    HARMONICSModes.append("Swing Instrument(SPICE Kernels)");
    HARMONICSModes.append("Swing Instrument(Target Pointing)");
    drawSunVec=drawEarthVec=isDrawCenterOfFOV=drawOrbit=GL_TRUE;
    isScanning=drawOverlay=inputxyz=isChangeVal=drawFootprints=isDrawShadow=isAutoDrawFootprints=isComputedSubpolygon=drewShadow=GL_FALSE;
    isComputeSubPolygon=GL_TRUE;
    isRestoredShadowData=GL_FALSE;
    defaultBoundaryCorners[0]=-0.006144;
    defaultBoundaryCorners[1]=0.006144;
    defaultBoundaryCorners[2]=-0.006144;
    defaultBoundaryCorners[3]=0.006144;
    defaultBoundaryCorners[4]=0.1204711614;
    rollDir="Up";
    pitchDir="Right";
    yawDir="CCW";
    interval=60;
    rollUL=rollLL=pitchUL=pitchLL=yawUL=yawLL=0.0;
    rollCurrentAngle=pitchCurrentAngle=yawCurrentAngle=0.0;
    rollVariation=pitchVariation=yawVariation=0.0;
    screenWidth=480;
    screenHeight=480;
    codec=QTextCodec::codecForLocale();
    subPolygonID=-1;

    isLine = false;

    dummyColor.setRgbF(0.0, 0.0, 0.0, 0.0);
    prepareFootprint();
    initCL();
}

drawGLWidget::~drawGLWidget(){

}

void drawGLWidget::initializeGL(){
    qglClearColor(Qt::black);  // 背景色指定
    glClearDepth(1.f);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_RESCALE_NORMAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightspe);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
}

void drawGLWidget::resizeGL(int width, int height){
    //  ビューポートの指定
    glViewport(0, 0, width, height);

    glGetIntegerv(GL_VIEWPORT, viewport);
    viewport[2]=screenWidth;
    viewport[3]=screenHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void drawGLWidget::paintGL(){
    if(isRecordSubPointProjection==GL_FALSE){
        glFrustum(-0.006144, 0.006144, -0.006144, 0.006144, 0.0001204711614, 1000);
        glGetFloatv(GL_PROJECTION_MATRIX, subPointProjection);
        isRecordSubPointProjection=GL_TRUE;
    }
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glLoadIdentity();

    if(SCParameters.length()==0){
        glFrustum(defaultBoundaryCorners[0], defaultBoundaryCorners[1], defaultBoundaryCorners[2], defaultBoundaryCorners[3], defaultBoundaryCorners[4], 1000);
    }
    else{
        SpiceInt id;
        for(int i=0; i<SCParameters.length(); i++){
            if(SCParameters.at(i).SCName==SCName){
                id=i;
            }
        }
        spacecraftParam SCParameter=SCParameters.at(id);
        if(SCParameter.getRegisteredInstNum()==0){
            glFrustum(defaultBoundaryCorners[0], defaultBoundaryCorners[1], defaultBoundaryCorners[2], defaultBoundaryCorners[3], defaultBoundaryCorners[4], 1000);
        }
        else{
            SpiceDouble boundaryCorners[4][3];
            SpiceDouble forcalLength=SCParameter.getForcalLengthAndBoundaryCorners(instName, boundaryCorners);

            glFrustum(boundaryCorners[1][0], boundaryCorners[0][0], boundaryCorners[2][0], boundaryCorners[3][0], forcalLength, 1000);
        }
    }

    SpiceChar action[] = "IGNORE";
    erract_c("SET", STRLEN, action);

    /*投影マッピングのため　使わない
    if(targetModel.target3DModel.isMapping){
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);

    }*/


    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    compElements.computePos(SCName, targetName, et, sun_pos, earth_pos, sc_pos, target_pos);
    compElements.computeAttitude(instId, targetFrame, et, bsight, up_inst, &r, &p, &y);
    compElements.compTargetRotateMatrix(targetName, targetFrame, et, targetRot);
    affineMatrix=QMatrix4x4(targetRot);
    affine.setToIdentity();
    affineMatrix=affineMatrix.transposed();
    affine.rotate(180, 0.0, 0.0, 1.0);
    affineMatrix=affine*affineMatrix;

    if((isChangeVal==GL_TRUE&&preET!=et)||(animation==GL_TRUE)){
        inputXpos=sc_pos[0];
        inputYpos=sc_pos[1];
        inputZpos=sc_pos[2];
        inputRoll=r;
        inputPitch=p;
        inputYaw=y;
        preET=et;
        isComputedSubpolygon=GL_FALSE;
    }

    if(preET!=et){
        isComputedSubpolygon=GL_FALSE;
    }

    if(isDrawShadow==GL_TRUE||isAutoDrawFootprints==GL_TRUE||(isComputedSubpolygon==GL_FALSE&&isComputeSubPolygon==GL_TRUE)){
        calculateAffineTranslate();
        compBoundingSphere();
        isComputedSubpolygon=GL_TRUE;
    }

    if(isAutoDrawFootprints==GL_TRUE&&animation==GL_TRUE){
        emit drawFootprints_signal();
    }

    if((isDrawShadow==GL_TRUE&&animation==GL_FALSE&&preET!=et&&drewShadow==GL_FALSE)||(isDrawShadow==GL_TRUE&&initializedFootprint==GL_TRUE)){
        targetModel.backupPolygonColor();
        if(initializedFootprint==GL_TRUE){
            targetModel.restoreShadowData();
            initializedFootprint=GL_FALSE;
            //targetModel.restoreShadowData();
        }
        else{
            if(drewShadow==GL_FALSE){
                drawShadow();
            }
        }
        drewShadow=GL_TRUE;
    }

    if(drewShadow==GL_TRUE&&isRestoredShadowData==GL_TRUE&&isOnceRestore==GL_FALSE){
        cout<<"restore"<<endl;
        targetModel.restoreShadowData();
        isOnceRestore=GL_TRUE;
    }

    if(glViewMode=="fovMode"){
        draw3Dperspective();
    }
    else if(glViewMode=="geometryMode"){
        glShadeModel(GL_FLAT);
        drawGeo();
        glShadeModel(GL_SMOOTH);
    }
    glDisable(GL_DEPTH_TEST);

    emit setWindowTime_signal(et);
    if(animation==GL_TRUE||(isChangeVal==GL_FALSE)){
        emit setWindowPosAndAtt_signal(posSC[0], posSC[1], posSC[2], r, p, y);
    }

    dis=compElements.computeDistance(posSC, target_pos);
    emit setWindowDisTargettoSC_signal(dis);
    dis=compElements.computeDistance(posSC, earth_pos);
    convrt_c(dis, "KM", "AU", &dis);
    emit setWindowDisEarthtoSC_signal(dis);
    emit setWindowIll_signal(phase, incidence, emission, foundIll);

    //SubPolygonは常に計算する
    if(isComputeSubPolygon==GL_TRUE){
        GLint id=compSubSCPolygon();
        emit getSubSCPoint_signal(id);
    }

    emit qglColor(Qt::white);
    emit renderText(10,420,"X: "+QString::number(posSC[0], 'f', 3)+"[km]");
    emit renderText(10,440,"Y: "+QString::number(posSC[1], 'f', 3)+"[km]");
    emit renderText(10,460,"Z: "+QString::number(posSC[2], 'f', 3)+"[km]");
    emit renderText(140,420,"Roll : "+QString::number(r, 'f', 3)+"[deg]");
    emit renderText(140,440,"Pitch: "+QString::number(p, 'f', 3)+"[deg]");
    emit renderText(140,460,"Yaw : "+QString::number(y, 'f', 3)+"[deg]");

}

void drawGLWidget::draw3Dperspective(){
    lightpos[0]=sun_pos[0];
    lightpos[1]=sun_pos[1];
    lightpos[2]=sun_pos[2];
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    if(harmonicsMode==HARMONICSModes.at(0)){
        if(isChangeVal==GL_FALSE){
            posSC[0]=sc_pos[0];
            posSC[1]=sc_pos[1];
            posSC[2]=sc_pos[2];
            bsightDir[0]=posSC[0]+bsight[0];
            bsightDir[1]=posSC[1]+bsight[1];
            bsightDir[2]=posSC[2]+bsight[2];
            upVec[0]=up_inst[0];
            upVec[1]=up_inst[1];
            upVec[2]=up_inst[2];
        }
        else{
            posSC[0]=inputXpos+SCPosTranslate[0];
            posSC[1]=inputYpos+SCPosTranslate[1];
            posSC[2]=inputZpos+SCPosTranslate[2];
            r=inputRoll+cameravec[0];
            p=inputPitch+cameravec[1];
            y=inputYaw+cameravec[2];

            compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);
            bsightDir[0]=posSC[0]+compBoresight[0];
            bsightDir[1]=posSC[1]+compBoresight[1];
            bsightDir[2]=posSC[2]+compBoresight[2];
            upVec[0]=up_instFromInput[0];
            upVec[1]=up_instFromInput[1];
            upVec[2]=up_instFromInput[2];
        }
    }
    if(harmonicsMode==HARMONICSModes.at(1)){
        if(isChangeVal==GL_FALSE){
            posSC[0]=sc_pos[0];
            posSC[1]=sc_pos[1];
            posSC[2]=sc_pos[2];
            y=179.9;
        }
        else{
            posSC[0]=inputXpos+SCPosTranslate[0];
            posSC[1]=inputYpos+SCPosTranslate[1];
            posSC[2]=inputZpos+SCPosTranslate[2];

            if(inputYaw==0.0||inputYaw==180){
                y=179.9+cameravec[2];
            }
            else{
                y=inputYaw+cameravec[2];
            }
        }
        p=atan2(posSC[0]-target_pos[0], posSC[2]-target_pos[2]);
        r=-atan2(posSC[1]-target_pos[1], posSC[2]-target_pos[2]);
        r*=RadToDeg;
        p*=RadToDeg;

        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        p=atan2(posSC[0]-target_pos[0], posSC[2]-target_pos[2]);
        r=-atan2(posSC[1]-target_pos[1], hypot(0.0, posSC[0]-target_pos[0]));

        bsightDir[0]=posSC[0]+compBoresight[0];
        bsightDir[1]=posSC[1]+compBoresight[1];
        bsightDir[2]=posSC[2]+compBoresight[2];
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }
    else if(harmonicsMode==HARMONICSModes.at(2)){
        posSC[0]=sc_pos[0];
        posSC[1]=sc_pos[1];
        posSC[2]=sc_pos[2];

        compSwing();

        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        bsightDir[0]=posSC[0]+bsight[0];
        bsightDir[1]=posSC[1]+bsight[1];
        bsightDir[2]=posSC[2]+bsight[2];
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }
    else if(harmonicsMode==HARMONICSModes.at(3)){
        posSC[0]=sc_pos[0];
        posSC[1]=sc_pos[1];
        posSC[2]=sc_pos[2];
        posSC[0]=0.0;
        posSC[1]=0.0;
        posSC[2]=4.0;

        p=atan2(posSC[0]-target_pos[0], posSC[2]-target_pos[2]);
        r=-atan2(posSC[1]-target_pos[1], posSC[2]-target_pos[2]);
        r*=RadToDeg;
        p*=RadToDeg;
        y=179.9;

        compSwing();
        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        bsightDir[0]=posSC[0]+compBoresight[0];
        bsightDir[1]=posSC[1]+compBoresight[1];
        bsightDir[2]=posSC[2]+compBoresight[2];
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }

    compElements.computeIllum(SCName, instId, et, targetName, &phase, &incidence, &emission, &foundIll);

    gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    drawVec(target_pos, earth_pos, sun_pos, drawEarthVec, drawSunVec);

    glEnable(GL_COLOR_MATERIAL);
    glPushMatrix();
      glTranslated(target_pos[0], target_pos[1], target_pos[2]);
      glMultMatrixf(affineMatrix.data());
      targetModel.drawTarget3DModel();
    glPopMatrix();
    glDisable(GL_COLOR_MATERIAL);

    if(isDrawCenterOfFOV==GL_TRUE){
        drawCenterofFOV();
    }

    if(drawOverlay==GL_TRUE){
        drawOverlayImage(readedfilename);
    }
}

void drawGLWidget::drawGeo(){
    lightpos[0]=sun_pos[0];
    lightpos[1]=sun_pos[1];
    lightpos[2]=sun_pos[2];
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);


    if(harmonicsMode==HARMONICSModes.at(0)){
        if(isChangeVal==false){
            posSC[0]=sc_pos[0];
            posSC[1]=sc_pos[1];
            posSC[2]=sc_pos[2];
            bsightDir[0]=sc_pos[0]+bsight[0];
            bsightDir[1]=sc_pos[1]+bsight[1];
            bsightDir[2]=sc_pos[2]+bsight[2];
            bsightVec.setX(bsight[0]);
            bsightVec.setY(bsight[1]);
            bsightVec.setZ(bsight[2]);
            upVec[0]=up_inst[0];
            upVec[1]=up_inst[1];
            upVec[2]=up_inst[2];
        }
        else{
            r=inputRoll+cameravec[0];
            p=inputPitch+cameravec[1];
            y=inputYaw+cameravec[2];

            compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

            posSC[0]=inputXpos+SCPosTranslate[0];
            posSC[1]=inputYpos+SCPosTranslate[1];
            posSC[2]=inputZpos+SCPosTranslate[2];
            bsightDir[0]=posSC[0]+compBoresight[0];
            bsightDir[1]=posSC[1]+compBoresight[1];
            bsightDir[2]=posSC[2]+compBoresight[2];
            bsightVec.setX(compBoresight[0]);
            bsightVec.setY(compBoresight[1]);
            bsightVec.setZ(compBoresight[2]);
            upVec[0]=up_instFromInput[0];
            upVec[1]=up_instFromInput[1];
            upVec[2]=up_instFromInput[2];
        }
    }
    if(harmonicsMode==HARMONICSModes.at(1)){
        if(isChangeVal==false){
            posSC[0]=sc_pos[0];
            posSC[1]=sc_pos[1];
            posSC[2]=sc_pos[2];
            y=179.9;
        }
        else{
            posSC[0]=inputXpos+SCPosTranslate[0];
            posSC[1]=inputYpos+SCPosTranslate[1];
            posSC[2]=inputZpos+SCPosTranslate[2];

            if(inputYaw==0.0||inputYaw==180){
                y=179.9+cameravec[2];
            }
            else{
                y=inputYaw+cameravec[2];
            }
        }
        p=atan2(posSC[0]-target_pos[0], posSC[2]-target_pos[2]);
        r=-atan2(posSC[1]-target_pos[1], hypot(0.0, posSC[0]-target_pos[0]));
        r*=RadToDeg;
        p*=RadToDeg;

        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        bsightDir[0]=compBoresight[0];
        bsightDir[1]=compBoresight[1];
        bsightDir[2]=compBoresight[2];
        bsightVec.setX(compBoresight[0]);
        bsightVec.setY(compBoresight[1]);
        bsightVec.setZ(compBoresight[2]);
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }
    else if(harmonicsMode==HARMONICSModes.at(2)){
        posSC[0]=sc_pos[0];
        posSC[1]=sc_pos[1];
        posSC[2]=sc_pos[2];

        compSwing();

        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        bsightDir[0]=posSC[0]+bsight[0];
        bsightDir[1]=posSC[1]+bsight[1];
        bsightDir[2]=posSC[2]+bsight[2];
        bsightVec.setX(compBoresight[0]);
        bsightVec.setY(compBoresight[1]);
        bsightVec.setZ(compBoresight[2]);
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }
    else if(harmonicsMode==HARMONICSModes.at(3)){
        posSC[0]=sc_pos[0];
        posSC[1]=sc_pos[1];
        posSC[2]=sc_pos[2];
        posSC[0]=0.0;
        posSC[1]=0.0;
        posSC[2]=3.0;

        p=atan2(posSC[0]-target_pos[0], posSC[2]-target_pos[2]);
        r=-atan2(posSC[1]-target_pos[1], posSC[2]-target_pos[2]);
        r*=RadToDeg;
        p*=RadToDeg;
        y=179.9;

        compSwing();

        compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

        bsightDir[0]=posSC[0]+compBoresight[0];
        bsightDir[1]=posSC[1]+compBoresight[1];
        bsightDir[2]=posSC[2]+compBoresight[2];
        bsightVec[0]=compBoresight[0];
        bsightVec[1]=compBoresight[1];
        bsightVec[2]=compBoresight[2];
        upVec[0]=up_instFromInput[0];
        upVec[1]=up_instFromInput[1];
        upVec[2]=up_instFromInput[2];
    }


    gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);

    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    glLoadIdentity();

    gluLookAt(0.0, 0.0, geometryTranslate[2]+10, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glDisable(GL_COLOR_MATERIAL);
    glPushMatrix();
      glRotated(geometryRotate[0], 1.0, 0.0, 0.0);
      glRotated(geometryRotate[1], 0.0, 1.0, 0.0);
      glRotated(geometryRotate[2], 0.0, 0.0, 1.0);

      //glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1 , 0xE0E0);
      glLineWidth(2.5);

      glPushMatrix();
      glBegin(GL_LINE_STRIP);
      qglColor(Qt::yellow);

      //視線がおかしかったため応急処置------------------------------
      SpiceDouble bsightVec_bd[3], originalBound[3];
      bsightVec_bd[0] = bsightVec[0];
      bsightVec_bd[1] = bsightVec[1];
      bsightVec_bd[2] = bsightVec[2];

      glVertex3d(posSC[0], posSC[1], posSC[2]);

      bsightVec_bd[0] *= 10.0;
      bsightVec_bd[1] *= 10.0;
      bsightVec_bd[2] *= 10.0;

      vadd_c(posSC, bsightVec_bd, originalBound);

      //SCと視線のベクトル化
      originalBound[0] -= posSC[0];
      originalBound[1] -= posSC[1];
      originalBound[2] -= posSC[2];

      //SCポジションにベクトルを足して、座標化
      vadd_c(posSC, originalBound, originalBound);

      //----------------------------------------------------------

      glVertex3d(originalBound[0], originalBound[1], originalBound[2]);

      glEnd();

      glPopMatrix();

      if(isLine){
      glBegin(GL_LINE_STRIP);
        qglColor(Qt::blue);
        glVertex3d(posSC[0], posSC[1], posSC[2]);
        glVertex3d(bsightVec[0]*10.0, bsightVec[1]*10.0, bsightVec[2]*10.0);

        glEnd();
      }

      drawVec(target_pos, earth_pos, sun_pos, drawEarthVec, drawSunVec);

      glDisable(GL_LINE_STIPPLE);
      glEnable(GL_LIGHTING);
      //glEnable(GL_DEPTH_TEST);

      glEnable(GL_COLOR_MATERIAL);
      glPushMatrix();
        glTranslated(posSC[0], posSC[1], posSC[2]);

        SCAffineMatrix.setToIdentity();
        SCAffineMatrix.rotate(r, 1.0, 0.0, 0.0);
        SCAffineMatrix.rotate(p, 0.0, 1.0, 0.0);
        SCAffineMatrix.rotate(y, 0.0, 0.0, 1.0);

        glPushMatrix();
          glMultMatrixf(SCAffineMatrix.data());
          glPushMatrix();
            glScaled(1.5, 1.5, 1.5);
            glRotated(180, 0.0, 0.0, 1.0);
            glRotated(90, 1.0, 0.0, 0.0);
            spacecraftModel.draw3DSCModel();
          glPopMatrix();
        glPopMatrix();
      glPopMatrix();

      glPushMatrix();
        glMultMatrixf(affineMatrix.data());
        targetModel.drawTarget3DModel();
      glPopMatrix();
      glDisable(GL_COLOR_MATERIAL);

    glPopMatrix();
}

void drawGLWidget::compSwing(){
    GLdouble v;
    if(step<interval){
        GLdouble t=step/interval;
        v=rollVariation*t;
        for(int i=0; i<(int)(interval/step); i++){
            if(rollVariation>0){
                if(rollDir=="Up"){
                    rollCurrentAngle+=v;
                }
                if(rollDir=="Down"){
                    rollCurrentAngle=rollCurrentAngle-v;
                    cout<<"Down"<<endl;
                }
                if((rollCurrentAngle)>rollUL){
                    rollCurrentAngle=rollUL;
                    cout<<"To down"<<endl;
                    rollDir="Down";
                }
                else if((rollCurrentAngle)<rollLL){
                    rollCurrentAngle=rollLL;
                    cout<<"To up"<<endl;
                    rollDir="Up";
                }
            }
            else if(rollVariation<0){
                if(rollDir=="Up"){
                    rollCurrentAngle=rollCurrentAngle-v;
                }
                if(rollDir=="Down"){
                    rollCurrentAngle=rollCurrentAngle+v;
                }
                if(rollCurrentAngle>rollUL){
                    rollCurrentAngle=rollUL;
                    rollDir="Down";
                }
                else if(rollCurrentAngle<rollLL){
                    rollCurrentAngle=rollLL;
                    rollDir="Up";
                }
            }

            v=pitchVariation*t;

            if(pitchVariation>0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle+=v;
                }
                if(rollDir=="Right"){
                    pitchCurrentAngle-=v;
                }
                if((pitchCurrentAngle)>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Left";
                }
                else if((pitchCurrentAngle)<pitchLL){
                    pitchCurrentAngle=pitchLL;
                    pitchDir="Right";
                }
            }
            else if(pitchVariation<0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle-=v;
                }
                if(pitchDir=="Right"){
                    pitchCurrentAngle+=v;
                }
                if(pitchCurrentAngle>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Right";
                }
                else if(pitchCurrentAngle<pitchLL){
                    pitchCurrentAngle=pitchLL;
                    pitchDir="Left";
                }
            }

            v=yawVariation*t;
            if(yawVariation>0){
                if(yawDir=="CCW"){
                    yawCurrentAngle+=v;
                }
                if(yawDir=="CW"){
                    yawCurrentAngle-=v;
                }
                if((yawCurrentAngle)>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="CW";
                }
                else if((yawCurrentAngle)<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="CCW";
                }
            }
            else if(yawVariation<0){
                if(yawDir=="Up"){
                    yawCurrentAngle-=v;
                }
                if(yawDir=="Down"){
                    yawCurrentAngle+=v;
                }
                if(yawCurrentAngle>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="Down";
                }
                else if(yawCurrentAngle<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="Up";
                }
            }

            r+=rollCurrentAngle;
            p+=pitchCurrentAngle;
            y+=yawCurrentAngle;

            if(isAutoDrawFootprints==GL_TRUE&&animation==GL_TRUE){
                compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);
                bsightDir[0]=posSC[0]+bsight[0];
                bsightDir[1]=posSC[1]+bsight[1];
                bsightDir[2]=posSC[2]+bsight[2];
                upVec[0]=up_instFromInput[0];
                upVec[1]=up_instFromInput[1];
                upVec[2]=up_instFromInput[2];
                gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);
                glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
                emit drawFootprints_signal();
                glLoadIdentity();
            }
        }
    }
    else if(step>interval){
        GLdouble s=step/interval;
        GLdouble a, rv, pv, yv;

        for(int i=0; i<(int)s; i++){
            if(rollVariation>0){
                if(rollDir=="Up"){
                    rollCurrentAngle+=rollVariation;
                }
                else if(rollDir=="Down"){
                    rollCurrentAngle-=rollVariation;
                }
                if(rollCurrentAngle>rollUL){
                    rollCurrentAngle=rollUL;
                    rollDir="Down";
                }
                else if(rollCurrentAngle<rollLL){
                    rollCurrentAngle=rollLL;
                    rollDir="Up";
                }
            }
            else if(rollVariation<0){
                if(rollDir=="Up"){
                    rollCurrentAngle-=rollVariation;
                }
                else if(rollDir=="Down"){
                    rollCurrentAngle+=rollVariation;
                }

                if(rollCurrentAngle>rollUL){
                    rollCurrentAngle=rollUL;
                    rollDir="Down";
                }
                else if(rollCurrentAngle<rollLL){
                    rollCurrentAngle=rollLL;
                    rollDir="Up";
                }
            }

            if(pitchVariation>0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle+=pitchVariation;
                }
                else if(pitchDir=="Right"){
                    pitchCurrentAngle-=pitchVariation;
                }
                if(pitchCurrentAngle>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Right";
                }
                else if(pitchCurrentAngle<pitchLL){
                    pitchCurrentAngle=rollLL;
                    pitchDir="Left";
                }
            }
            else if(pitchVariation<0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle-=pitchVariation;
                }
                else if(pitchDir=="Right"){
                    pitchCurrentAngle+=pitchCurrentAngle;
                }
                if(pitchCurrentAngle>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Right";
                }
                else if(pitchCurrentAngle<pitchLL){
                    pitchCurrentAngle=pitchLL;
                    pitchDir="Left";
                }
            }

            if(yawVariation>0){
                if(yawDir=="CCW"){
                    yawCurrentAngle+=yawVariation;
                }
                else if(yawDir=="CW"){
                    yawCurrentAngle-=yawVariation;
                }
                if(yawCurrentAngle>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="CW";
                }
                else if(yawCurrentAngle<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="CCW";
                }
            }
            else if(yawVariation<0){
                if(yawDir=="CCW"){
                    yawCurrentAngle-=yawVariation;
                }
                else if(yawDir=="CW"){
                    yawCurrentAngle+=yawVariation;
                }
                if(yawCurrentAngle>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="CW";
                }
                else if(yawCurrentAngle<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="CCW";
                }
            }
            r+=rollCurrentAngle;
            p+=pitchCurrentAngle;
            y+=yawCurrentAngle;

            if(isAutoDrawFootprints==GL_TRUE&&animation==GL_TRUE){
                compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);
                bsightDir[0]=posSC[0]+bsight[0];
                bsightDir[1]=posSC[1]+bsight[1];
                bsightDir[2]=posSC[2]+bsight[2];
                upVec[0]=up_instFromInput[0];
                upVec[1]=up_instFromInput[1];
                upVec[2]=up_instFromInput[2];
                gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);
                glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
                emit drawFootprints_signal();
                glLoadIdentity();
            }
        }

        a=(GLint)s-s;
        if(a!=0.0){
            if(a<0){
                a=(-a);
            }
            rv=rollVariation*a;
            pv=pitchVariation*a;
            yv=yawVariation*a;

            if(rollVariation>0){
                if(rollDir=="Up"){
                    rollCurrentAngle+=rv;
                }
                else if(rollDir=="Down"){
                    rollCurrentAngle-=rv;
                }
                if(rollCurrentAngle>rollUL){
                    rollCurrentAngle=rollUL;
                    rollDir="Down";
                }
                else if(rollCurrentAngle<rollLL){
                    rollCurrentAngle=rollLL;
                    rollDir="Up";
                }
            }
            else if(rollVariation<0){
                if(rollDir=="Up"){
                    rollCurrentAngle-=rv;
                }
                else if(rollDir=="Down"){
                    rollCurrentAngle+=rv;
                }

                if(rollCurrentAngle>rollUL){
                    rollCurrentAngle=rollUL;
                    rollDir="Down";
                }
                else if(rollCurrentAngle<rollLL){
                    rollCurrentAngle=rollLL;
                    rollDir="Up";
                }
            }
            if(pitchVariation>0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle+=pv;
                }
                else if(pitchDir=="Right"){
                    pitchCurrentAngle-=pv;
                }
                if(pitchCurrentAngle>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Right";
                }
                else if(pitchCurrentAngle<pitchLL){
                    pitchCurrentAngle=rollLL;
                    pitchDir="Left";
                }
            }
            else if(pitchVariation<0){
                if(pitchDir=="Left"){
                    pitchCurrentAngle-=pv;
                }
                else if(pitchDir=="Right"){
                    pitchCurrentAngle+=pv;
                }
                if(pitchCurrentAngle>pitchUL){
                    pitchCurrentAngle=pitchUL;
                    pitchDir="Right";
                }
                else if(pitchCurrentAngle<pitchLL){
                    pitchCurrentAngle=pitchLL;
                    pitchDir="Left";
                }
            }

            if(yawVariation>0){
                if(yawDir=="CCW"){
                    yawCurrentAngle+=yv;
                }
                else if(yawDir=="CW"){
                    yawCurrentAngle-=yv;
                }
                if(yawCurrentAngle>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="CW";
                }
                else if(yawCurrentAngle<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="CCW";
                }
            }
            else if(yawVariation<0){
                if(yawDir=="CCW"){
                    yawCurrentAngle-=yv;
                }
                else if(yawDir=="CW"){
                    yawCurrentAngle+=yv;
                }
                if(yawCurrentAngle>yawUL){
                    yawCurrentAngle=yawUL;
                    yawDir="CW";
                }
                else if(yawCurrentAngle<yawLL){
                    yawCurrentAngle=yawLL;
                    yawDir="CCW";
                }
            }
        }
        r+=rollCurrentAngle;
        p+=pitchCurrentAngle;
        y+=yawCurrentAngle;
    }
    else if(step==interval){
        if(rollVariation>0){
            if(rollDir=="Up"){
                rollCurrentAngle+=rollVariation;
            }
            else if(rollDir=="Down"){
                rollCurrentAngle-=rollVariation;
            }
            if(rollCurrentAngle>=rollUL){
                rollCurrentAngle=rollUL;
                rollDir="Down";
            }
            else if(rollCurrentAngle<=rollLL){
                rollCurrentAngle=rollLL;
                rollDir="Up";
            }
        }
        else if(rollVariation<0){
            if(rollDir=="Up"){
                rollCurrentAngle-=rollVariation;
            }
            else if(rollDir=="Down"){
                rollCurrentAngle+=rollVariation;
            }
            if(rollCurrentAngle>=rollUL){
                rollCurrentAngle=rollUL;
                rollDir="Down";
            }
            else if(rollCurrentAngle<=rollLL){
                rollCurrentAngle=rollLL;
                rollDir="Up";
            }
        }

        if(pitchVariation>0){
            if(pitchDir=="Left"){
                pitchCurrentAngle+=pitchVariation;
            }
            else if(pitchDir=="Right"){
                pitchCurrentAngle-=pitchVariation;
            }
            if(pitchCurrentAngle>pitchUL){
                pitchCurrentAngle=pitchUL;
                pitchDir="Right";
            }
            else if(pitchCurrentAngle<pitchLL){
                pitchCurrentAngle=rollLL;
                pitchDir="Left";
            }
        }
        else if(pitchVariation<0){
            if(pitchDir=="Left"){
                pitchCurrentAngle-=pitchVariation;
            }
            else if(pitchDir=="Right"){
                pitchCurrentAngle+=pitchCurrentAngle;
            }
            if(pitchCurrentAngle>pitchUL){
                pitchCurrentAngle=pitchUL;
                pitchDir="Right";
            }
            else if(pitchCurrentAngle<pitchLL){
                pitchCurrentAngle=pitchLL;
                pitchDir="Left";
            }
        }

        if(yawVariation>0){
            if(yawDir=="CCW"){
                yawCurrentAngle+=yawVariation;
            }
            else if(yawDir=="CW"){
                yawCurrentAngle-=yawVariation;
            }
            if(yawCurrentAngle>yawUL){
                yawCurrentAngle=yawUL;
                yawDir="CW";
            }
            else if(yawCurrentAngle<yawLL){
                yawCurrentAngle=yawLL;
                yawDir="CCW";
            }
        }
        else if(yawVariation<0){
            if(yawDir=="CCW"){
                yawCurrentAngle-=yawVariation;
            }
            else if(yawDir=="CW"){
                yawCurrentAngle+=yawVariation;
            }
            if(yawCurrentAngle>yawUL){
                yawCurrentAngle=yawUL;
                yawDir="CW";
            }
            else if(yawCurrentAngle<yawLL){
                yawCurrentAngle=yawLL;
                yawDir="CCW";
            }
        }
        r+=rollCurrentAngle;
        p+=pitchCurrentAngle;
        y+=yawCurrentAngle;
    }
}

//void drawGLWidget::drawScreen(GLuint screenN, QString instName, SpiceDouble boundary[]){

//}

void drawGLWidget::drawInstName(QString instName){
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glRasterPos3d(-0.45, -1.7, 0.5);
    glColor3d(1.0, 1.0, 1.0);
    string str=instName.toStdString();
    int size=(int)str.size();
    for(int i = 0; i < size; ++i){
        char ic = str[i];
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ic);
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

//void drawGLWidget::drawDividingLine(GLuint num){

//}

void drawGLWidget::setLoadSCModelFile_slot(QString name){
    spacecraftModel.setSCModelFile(name);
}

void drawGLWidget::setLoadTargetModelFile_slot(QString name){
    targetModel.setTargetModelFile(name);
}

void drawGLWidget::setLoadSCMTLFile_slot(QString name){
    spacecraftModel.setSCMTLFile(name);
}

void drawGLWidget::setLoadTargetMTLFile_slot(QString name){
    targetModel.setTargetMTLFile(name);
}

void drawGLWidget::isComputeSubPolygon_slot(GLboolean isComputeSubPolygon){
    this->isComputeSubPolygon=isComputeSubPolygon;
    emit getSubSCPoint_signal(-1);
    updateGL();
}

void drawGLWidget::getResolutionOfModel_slot(){
    QString res=QString::number(targetModel.target3DModel.faceNum);
    emit getResolution_signal(res);
}

void drawGLWidget::visualizationObservedData_slot(QStringList observedPolygon){
    targetModel.visualizePolygon(observedPolygon);
    updateGL();
}

void drawGLWidget::keyPressEvent(QKeyEvent *event){
    switch(event->key()){
    case Qt::Key_X:
        if(event->modifiers() & Qt::ShiftModifier){
            if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
                SCPosTranslate[0]-=0.001;
            else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
                SCPosTranslate[0]-=0.001;
        }    
        else{
            if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
                SCPosTranslate[0]+=0.001;
            else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
                 SCPosTranslate[0]+=0.001;
        }
        break;

    case Qt::Key_Y:
      if(event->modifiers() & Qt::ShiftModifier){
        //rotatexyz[1]-=5;
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[1]-=0.001;
          else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[1]-=0.001;
      }
      else{
        //rotatexyz[1]+=5;
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[1]+=0.001;
          else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[1]+=0.001;
      }
        break;

    case Qt::Key_Z:
      if(event->modifiers() & Qt::ShiftModifier){
        //rotatexyz[2]-=5;
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[2]-=0.001;
          else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[2]-=0.001;
      }

      else{
          //rotatexyz[2]+=5;
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[2]+=0.001;
          else if(glViewMode=="geometryMode"&&isChangeVal==GL_TRUE)
              SCPosTranslate[2]+=0.001;
          }
        break;

    case Qt::Key_C:
        break;

    case Qt::Key_R:
      if(event->modifiers() & Qt::ShiftModifier){
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&(harmonicsMode=="OperationFreeChangeMode")*/)
              cameravec[0]-=0.01;
      }

      else{
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&harmonicsMode==("OperationFreeChangeMode")*/)
              cameravec[0]+=0.01;
      }
        break;

    case Qt::Key_P:
      if(event->modifiers() & Qt::ShiftModifier){
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&(harmonicsMode=="OperationFreeChangeMode")*/)
              cameravec[1]-=0.01;
      }

      else{
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&harmonicsMode==("OperationFreeChangeMode")*/)
              cameravec[1]+=0.01;
      }
        break;

    case Qt::Key_T:
      if(event->modifiers() & Qt::ShiftModifier){
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&(harmonicsMode=="OperationPointingTargetMode"||harmonicsMode=="OperationFreeChangeMode")*/)
              cameravec[2]-=0.1;
      }
      else{
          if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE/*&&(harmonicsMode=="OperationPointingTargetMode"||harmonicsMode=="OperationFreeChangeMode")*/)
              cameravec[2]+=0.1;    
      }
      break;

    case Qt::Key_I:
        if(glViewMode=="fovMode"&&isChangeVal==GL_TRUE){
            SCPosTranslate[0]=SCPosTranslate[1]=SCPosTranslate[2]=0;
            cameravec[0]=cameravec[1]=cameravec[2]=0;
            emit changeKeyStroke_signal(sc_pos[0], sc_pos[1], sc_pos[2], r, p, y);
        }
        break;

    default:
        break;
    }
    updateGL();
}

void drawGLWidget::mousePressEvent(QMouseEvent *event){
    lastPos=event->pos();
}

void drawGLWidget::mouseMoveEvent(QMouseEvent *event){
    GLfloat dx=GLfloat(event->x()-lastPos.x())/width();
    GLfloat dy=GLfloat(event->y()-lastPos.y())/height();

    if(event->buttons()&Qt::RightButton){
        if(glViewMode=="observerMode"){
            observerRotate[0]+=180*dy;
            observerRotate[1]+=180*dx;
        }

        else if(glViewMode=="targetMode"){
            targetRotate[0]+=180*dy;
            targetRotate[1]+=180*dx;
        }
        else if(glViewMode=="geometryMode"){
            geometryTranslate[0]+=dy;
            geometryTranslate[1]-=dx;
        }
    }

    if(event->buttons()&Qt::LeftButton){
        if(glViewMode=="observerMode"){
            observerTranslate[0]+=dx;
            observerTranslate[1]-=dy;
        }

        else if(glViewMode=="targetMode"){
            targetTranslate[0]+=dx;
            targetTranslate[1]-=dy;
        }

        else if(glViewMode=="geometryMode"){
            geometryRotate[0]+=180*dy;
            geometryRotate[1]+=180*dx;
            geometryRotate[2]+=90*dy + 90*dx;
        }

    }
    lastPos=event->pos();

    updateGL();
}

void drawGLWidget::viewMode_slot(QString mode){
    if(glViewMode!=mode){
        glViewMode=mode;
        updateGL();
    }
}

void drawGLWidget::setIsChangeVal_slot(GLboolean changeval){
    //printf("isChangeVal: %d\n", changeval);
    if(preET!=et && changeval==GL_TRUE){
        inputXpos=sc_pos[0];
        inputYpos=sc_pos[1];
        inputZpos=sc_pos[2];
        inputRoll=r;
        inputPitch=p;
        inputYaw=y;
        //preET=et;
    }

    isChangeVal=changeval;
    updateGL();
}

void drawGLWidget::setAutoDrawFootprintsDuringScan_slot(GLboolean isAutoDrawFootprints){
    this->isAutoDrawFootprints=isAutoDrawFootprints;
}

void drawGLWidget::wheelEvent(QWheelEvent *event){
    if(glViewMode=="geometryMode")
        geometryTranslate[2]+=(double)event->delta()/120;

    else if(glViewMode=="fovMode"&&harmonicsMode==HARMONICSModes.at(1))
        SCPosTranslate[2]+=(double)event->delta()/120/1000;

    updateGL();
}

void drawGLWidget::timerEvent(QTimerEvent *event){
    Q_UNUSED(event);
    if(et<end_et&&animation==GL_TRUE){
        et+=step;
        preET=et;
        drewShadow=GL_FALSE;
        isRestoredShadowData=GL_FALSE;
    }
    else if(et>=end_et){
        animation=GL_FALSE;
        emit finishedAnim_signal();
    }

    updateGL();
}

void drawGLWidget::startAnimation(){
    animation=GL_TRUE;
    if(isDrawShadow==GL_TRUE){
        targetModel.initTargetPolygonColor("Record");
    }
    glShadeModel(GL_FLAT);
    timer->start(200, this);
}

void drawGLWidget::stopAnimation(){
    animation=GL_FALSE;
    glShadeModel(GL_SMOOTH);

    if(isDrawShadow==GL_TRUE){
        drawShadow();
        drewShadow=GL_TRUE;
    }
    timer->stop();
}

void drawGLWidget::setSwingParameters_slot(SpiceInt interval, SpiceDouble rollLL, SpiceDouble rollUL, SpiceDouble rollVariation, SpiceDouble pitchLL, SpiceDouble pitchUL, SpiceDouble pitchVariation, SpiceDouble yawLL, SpiceDouble yawUL, SpiceDouble yawVariation){
    this->interval=interval;
    this->rollUL=rollUL;
    this->rollLL=rollLL;
    this->rollVariation=rollVariation;
    this->pitchUL=pitchUL;
    this->pitchLL=pitchLL;
    this->pitchVariation=pitchVariation;
    this->yawUL=yawUL;
    this->yawLL=yawLL;
    this->yawVariation=yawVariation;

    rollmid=(rollUL+rollLL)/2;
    pitchmid=(pitchUL+pitchLL)/2;
    yawmid=(yawUL+yawLL)/2;
    rollCurrentAngle=rollmid;
    pitchCurrentAngle=pitchmid;
    yawCurrentAngle=yawmid;
}

//画像の保存
void drawGLWidget::saveImage_slot(){
    savefilename=QFileDialog::getSaveFileName(this, tr("Save Screen Image(JPG JPEG PNG BMP TIFF FITS FIT FTS)"),QDir::homePath(), tr("Image File(*.jpg *.jpeg *.png *.bmp *.tiff *.fits *.fit. *.fts)"));
    if(savefilename.isEmpty())
        return ;

    //一つ前のバッファが記録されないように再描画する。
    updateGL();

    QFileInfo fileinfo;
    fileinfo.setFile(savefilename);
    QString fileExtension=fileinfo.suffix();
    GLint physicalViewport[4];
    glGetIntegerv(GL_VIEWPORT, physicalViewport);

    QImage saveImg=grabFrameBuffer(true).copy(0, 0, width()*(physicalViewport[2]/width()), height()*(physicalViewport[3]/height()));
    QImage scaledSaveImg=saveImg.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::FastTransformation);
    if(fileExtension=="fit"||fileExtension=="fits"||fileExtension=="fts"){

        SpiceLong naxes[]={imageWidth, imageHeight};
        SpiceLong naxis=2;
        GLuint nelements(1);
        QTransform transformM;

        scaledSaveImg=scaledSaveImg.transformed(transformM.rotate(180, Qt::XAxis), Qt::FastTransformation);
        scaledSaveImg=scaledSaveImg.convertToFormat(QImage::Format_RGB32);

        nelements=accumulate(&naxes[0], &naxes[naxis], 1, multiplies<GLuint>());
        GLuint *bits=(GLuint *)scaledSaveImg.bits();

        cout<<scaledSaveImg.bitPlaneCount()<<endl;
        valarray<int> contents(naxes[0]*naxes[1]);
        SpiceLong  fpixel(1);
        auto_ptr<FITS> saveFitsFile(0);

        try{
            saveFitsFile.reset(new FITS(savefilename.toStdString().c_str(), ULONG_IMG, naxis, naxes));

            for(int i=naxes[0]-1; i>=0; i--){
                for(int j=0; j<naxes[1]; j++){
                    contents[(naxes[1]-1-i)*naxes[1]+j]=bits[(naxes[1]-1-j)*naxes[1]+i];
                }
            }

            QString str;
            SpiceDouble tick;
//            SpiceBoolean found;
            et2utc_c(et, "ISOC", 0, STRLEN, utc);
            sce2c_c(SCCode.toInt(), et, &tick);
            saveFitsFile->pHDU().addKey("ax1", imageWidth,"pixel");
            saveFitsFile->pHDU().addKey("ax2", imageHeight,"pixel");

            saveFitsFile->pHDU().addKey("TI_0", tick,"Spacecraft Clock");
            saveFitsFile->pHDU().write(fpixel, nelements, contents);
        }
        catch(FITS::CantCreate){
            cout<<"Can't save fits image file"<<endl;
            return ;
        }
    }
    else{
        scaledSaveImg=scaledSaveImg.convertToFormat(QImage::Format_RGB32);

        scaledSaveImg.save(savefilename, 0, -1);
    }
}

void drawGLWidget::drawVec(SpiceDouble target_pos[3], SpiceDouble earth_pos[3], SpiceDouble sun_pos[3], GLboolean drawEarthVec, GLboolean drawSunVec){
    glPushMatrix();
      //glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);

      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1 , 0xE0E0);
      glLineWidth(2.5);

      //earth-sc vector
      if(drawEarthVec==GL_TRUE){
          glBegin(GL_LINES);
          qglColor(Qt::green);
          glVertex3d(earth_pos[0], earth_pos[1], earth_pos[2]);
          glVertex3d(target_pos[0], target_pos[0], target_pos[0]);
          glEnd();
      }

       //sun-target
       if(drawSunVec==GL_TRUE){
           glBegin(GL_LINES);
           qglColor(Qt::red);
           glVertex3d(sun_pos[0], sun_pos[1], sun_pos[2]);
           glVertex3d(target_pos[0], target_pos[0], target_pos[0]);
           glEnd();
       }

       // axis of rotation
       glBegin(GL_LINES);
       qglColor(Qt::white);
       glVertex3d(0.0, -10.0, 0.0);
       glVertex3d(0.0, 10.0, 0.0);
       glEnd();

       glLineWidth(1.0);
       glDisable(GL_LINE_STIPPLE);
       glEnable(GL_LIGHTING);
     glPopMatrix();
}

void drawGLWidget::drawCenterofFOV(){
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
        glLoadIdentity();
        glPushMatrix();
         glBegin(GL_LINE_STRIP);
         qglColor(Qt::yellow);

         glVertex3d(0.0, 0.04, 1.0);
         glVertex3d(0.04, 0.0, 1.0);
         glVertex3d(0.0, -0.04, 1.0);
         glVertex3d(-0.04, 0.0, 1.0);
         glVertex3d(0.0, 0.04, 1.0);

         glEnd();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
       glPopMatrix();
       glMatrixMode(GL_MODELVIEW);
     glPopMatrix();
   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);
}

void drawGLWidget::drawOrbit_slot(GLboolean draworbit){
    drawOrbit=draworbit;
    updateGL();
}

void drawGLWidget::drawOrbital(){
    GLUquadric *qobj=gluNewQuadric();
    gluQuadricDrawStyle(qobj, GLU_FILL);
    gluQuadricNormals(qobj, GLU_SMOOTH);
    gluQuadricTexture(qobj, GL_TRUE);

    glEnable(GL_TEXTURE_2D);
    for(int i=0; i<orbitList.length(); i++){
        QVector3D orbitVec=orbitList.at(i);

        glPushMatrix();
         gluSphere(qobj, 0.5, 24, 24);
        glPopMatrix();
    }
}

void drawGLWidget::drawAreaOfFOV_slot(GLboolean areaOfFOV){
    drawAreaOfFOV=areaOfFOV;
    targetModel.isDrawFootprints=areaOfFOV;
    updateGL();
}

//void drawGLWidget::drawAreaOfFOV_on3DView(SpiceDouble roll, SpiceDouble pitch, SpiceDouble yaw, GLboolean drawArea){
//    if(drawArea==GL_TRUE){

//    }
//}

QString drawGLWidget::getfileName(){
    return savefilename;
}

void drawGLWidget::setMode_slot(QString mode){
    harmonicsMode=mode;

    if(harmonicsMode==HARMONICSModes.at(2)||harmonicsMode==HARMONICSModes.at(3)){
        isScanning=GL_TRUE;
    }
    else{
        isScanning=GL_FALSE;
    }
    updateGL();
}

void drawGLWidget::setSC_slot(QString sc){
    SCName=sc;
    SpiceBoolean found;
    SpiceInt code;
    bodn2c_c(SCName.toStdString().c_str(), &code, &found);
    SCCode=QString::number(code);
    SpiceBoolean reg=SPICEFALSE;
    for(int i=0; i<SCParameters.length(); i++){
        if(SCParameters.at(i).SCName==SCName){
            reg=SPICETRUE;
            break;
        }
    }

    if(reg==SPICEFALSE){
        spacecraftParam scparam(sc);
        SCParameters.append(scparam);
    }
}

void drawGLWidget::setFrame_slot(QString frame){
    targetFrame=frame;
    updateGL();
}

//探査機の名前を取得して、渡す。探査機名をコードに直接書かない。
void drawGLWidget::setInst_slot(QString instName){
    //cout<<"setInst"<<endl;
    this->instName=instName;
    SpiceBoolean isSCReg, isInstReg, found;
    SpiceInt instID, id;

    //IKファイルが探査機のSPKやSCKより先に単独で読み込まれる場合がある。登録されたかどうか確認する必要がある。
    bodn2c_c(instName.toStdString().c_str(), &instID, &found);

    if(!found){
       printf("Instrument name %s could not be translated to an ID code\n", instName.toStdString().c_str());
    }

    isSCReg=SPICEFALSE;
    for(int i=0; i<SCParameters.length(); i++){
        if(SCParameters.at(i).SCName==SCName){
            id=i;
            isSCReg=SPICETRUE;
            break;
        }
    }

    spacecraftParam SCParameter;
    if(isSCReg==SPICETRUE){
        SCParameter=SCParameters.at(id);
        isInstReg=SCParameter.isInstRegistered(instName);
    }
    if(isSCReg==SPICETRUE && isInstReg==SPICEFALSE){
        SCParameter.setInstParam(instName);
        SCParameters[id]=SCParameter;
        QColor color=SCParameter.getFootprintColorCode(instName);
        QString colorName=SCParameter.getFootprintColorName(instName);
        emit setInstrumentsParameters_signal(instName, QString::number(instID), SCParameter.getInstType(instName), QString::number(SCParameter.getFOVAngle(instName)), colorName, color);
    }

   updateGL();
}

//void drawGLWidget::setFootprintsOfinstOnSC_slot(QString instName, SpiceBoolean isDraw){
//}

void drawGLWidget::setTarget_slot(QString target){
    targetName=target;
}

void drawGLWidget::setTime_slot(SpiceDouble starttime, SpiceDouble endtime, SpiceDouble Animstep){
    if(step!=Animstep&&start_et==starttime&&end_et==endtime){
        step=Animstep;
        return ;
    }
    else if(start_et!=starttime&&end_et==endtime&&step==Animstep){
        start_et=starttime;
        et=starttime;
    }
    else if(start_et==starttime&&end_et!=endtime&&step==Animstep){
        end_et=endtime;
        return ;
    }
    else{
        start_et=starttime;
        end_et=endtime;
        et=starttime;
        step=Animstep;
    }
    updateGL();
}

void drawGLWidget::setInputPosAndAtt_slot(SpiceDouble posX, SpiceDouble posY, SpiceDouble posZ, SpiceDouble roll, SpiceDouble pitch, SpiceDouble yaw){    
    inputXpos=posX;
    inputYpos=posY;
    inputZpos=posZ;
    inputRoll=roll;
    inputPitch=pitch;
    inputYaw=yaw;

    if(animation==GL_FALSE){
        updateGL();
    }
}

void drawGLWidget::sendInstruments_slot(QString SCName,QStringList instNames){
    SpiceBoolean isSCReg, isInstReg, found;
    SpiceInt instID, id;

    for(int i=0; i<instNames.length(); i++){
        //IKファイルが探査機のSPKやSCKより先に単独で読み込まれる場合がある。登録されたかどうか確認する必要がある。
        bodn2c_c(instNames.at(i).toStdString().c_str(), &instID, &found);

        if(!found){
           printf("Instrument name %s could not be translated to an ID code\n", instName.toStdString().c_str());
        }

        isSCReg=SPICEFALSE;
        for(int i=0; i<SCParameters.length(); i++){
            if(SCParameters.at(i).SCName==SCName){
                id=i;
                isSCReg=SPICETRUE;
                break;
            }
        }

        spacecraftParam SCParameter;
        if(isSCReg==SPICETRUE){
            SCParameter=SCParameters.at(id);
            isInstReg=SCParameter.isInstRegistered(instName);
        }
        if(isSCReg==SPICETRUE && isInstReg==SPICEFALSE){
            SCParameter.setInstParam(instName);
            SCParameters[id]=SCParameter;
            QColor color=SCParameter.getFootprintColorCode(instName);
            QString colorName=SCParameter.getFootprintColorName(instName);
            emit setInstrumentsParameters_signal(instName, QString::number(instID), SCParameter.getInstType(instName), QString::number(SCParameter.getFOVAngle(instName)), colorName, color);
        }
    }
}

SpiceInt drawGLWidget::compSubSCPolygon(){
    subPolygonID=-1;

    MygluUnProject("SubPoint", modelView, subPointProjection, viewport);
    crossingDetection("Boresight", dummyColor, GL_TRUE);
    return subPolygonID;
}

void drawGLWidget::stepChanged_slot(GLint stepvalue){
    et=et+stepvalue;
    SCPosTranslate[0]=SCPosTranslate[1]=SCPosTranslate[2]=0.0;
    cameravec[0]=cameravec[1]=cameravec[2]=0.0;
    drewShadow=GL_FALSE;
    isRestoredShadowData=GL_FALSE;
    isOnceRestore=GL_FALSE;

    if(animation==GL_FALSE){
        updateGL();   
    }
}

void drawGLWidget::drawSunVec_slot(GLboolean isDrawVec){
    drawSunVec=isDrawVec;
    updateGL();
}

void drawGLWidget::drawEarthVec_slot(GLboolean isDrawVec){
    drawEarthVec=isDrawVec;
    updateGL();
}

void drawGLWidget::drawCenterOfFOV_slot(GLboolean isDrawCenOfFOV){
    isDrawCenterOfFOV=isDrawCenOfFOV;
    updateGL();
}

void drawGLWidget::isDrawFootprints_slot(GLboolean isDrawFootprints){
    drawFootprints=isDrawFootprints;
    targetModel.isDrawFootprints=isDrawFootprints;
}

void drawGLWidget::initializeFootprintsInfo_slot(){
    if(isDrawShadow==GL_FALSE&&drewShadow==GL_FALSE){
        targetModel.initTargetPolygonColor("ALLInit");
    }
    else{
        targetModel.initTargetPolygonColor("Record");
    }

    initializedFootprint=GL_TRUE;
    updateGL();
}

//設定してある機器でシミュレーション結果を表示するのか、もし画像ファイルから機器名が判明した場合、その機器に設定し直して表示するのか要検討
void drawGLWidget::setReadedfile_slot(QString filename){
    readedfilename=filename;
//    SpiceChar utc[STRLEN];
    SpiceDouble fileEt;

    QFileInfo fileinfo;
    fileinfo.setFile(readedfilename);
    QString ext=fileinfo.suffix();
    ext=ext.toLower();
    if(ext=="fit"||ext=="fits"||ext=="fts"){
        auto_ptr<FITS> pInfile(0);
//        SpiceDouble ti;

        try{
            pInfile.reset(new CCfits::FITS(filename.toStdString().c_str(), CCfits::Read, true));
            PHDU& fitsImage=pInfile->pHDU();
            string UTC_0;
            fitsImage.readKey("UTC_0", UTC_0);
            utc2et_c(UTC_0.c_str(), &fileEt);
            et=fileEt;
        }
        catch(FitsException& issue){
            cout<<"Can't open fits file"<<endl;
            //exit(2);
        }
    }
    else{
        QStringList list=readedfilename.split("_");
        if(list.length()>=2){
            scs2e_c(SCCode.toInt(), list.at(1).toStdString().c_str(), &fileEt);
            if(fileEt!=0)
            et=fileEt;
        }
    }
    updateGL();
}

void drawGLWidget::loadcontorImage_slot(QImage img, QString filetype){
    loadedImageFile=img;
    loadedFiletype=filetype;
}

void drawGLWidget::drawOverlay_slot(GLboolean drawOverlay){
    this->drawOverlay=drawOverlay;
    updateGL();
}

void drawGLWidget::drawOverlayImage(QString filename){
    if(filename.isEmpty()){
        return;
    }

    glEnable(GL_BLEND);

    QImage srcImg=loadedImageFile;
    QRgb color;
    int redcolor;

    for(int i=0; i<srcImg.height(); i++){
        for(int j=0; j<srcImg.width(); j++){
            color=srcImg.pixel(j, i);

            redcolor=qRed(color);
            QColor c;
            c.setRgb(redcolor, 0.0, 0.0, 0);
            srcImg.setPixel(j, i, c.rgb());
        }
    }
    QImage glImage=QGLWidget::convertToGLFormat(srcImg);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
        glLoadIdentity();
        glPushMatrix();
          glDrawPixels(glImage.width(), glImage.height(), GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

}

void drawGLWidget::createLogFile_slot(GLboolean create){
    logFileCreate=create;
}

void drawGLWidget::prepareFootprint(){
    GLint w, h;
    QString cw="LeftToRight";
    w=h=0;
    WandH.clear();

    subSCPointPolygonArea.append(QVector4D(screenWidth/2, screenHeight/2, 1, 1));
    while(1){
        if(cw=="LeftToRight"){    
            FOVArea.append(QVector4D(w, screenHeight-h+1, 1, 1));
            w++;
            if(w==(screenWidth-1)){
                cw="TopToBottom";
                //cw="STOP";
            }
        }

        else if(cw=="TopToBottom"){
            FOVArea.append(QVector4D(w-1, screenHeight-h, 1, 1));
            h++;

            if(h==(screenHeight-1)){
                cw="RightToLeft";
                //cw="STOP";
            }
        }

        else if(cw=="RightToLeft"){
            FOVArea.append(QVector4D(w, screenHeight-h-1, 1, 1));
            w--;

            if(w==0){
                cw="BottomToTop";
            }
        }

        else if(cw=="BottomToTop"){
            FOVArea.append(QVector4D(w+1, screenHeight-h, 1, 1));
            h--;

            if(h==0){
                cw="STOP";
            }
        }

        else if(cw=="STOP"){
            break;
        }
    }

    scientificInstrumentsPeriod.append(WandH.length());

    //Boresight
    for(int i=238; i<242; i++){
        for(int j=238; j<242; j++){
            boresightArea.append(QVector4D(i, (screenHeight)-j, 2, 1));
        }
    }
}

void drawGLWidget::compBoundingSphere(){
    //親と子のバウンデイングスフィアの中心座標を格納
   targetModel.spheresCenter.clear();

    //Root sphere
   QVector4D Rootshpere=affineMatrix*targetModel.rootSphere.toVector4D();
   targetModel.spheresCenter.append(Rootshpere);

    //Node spheres
    QVector4D affineVec;
    for(int i=0; i<targetModel.nodeLevel3Spheres.length(); i++){
        affineVec=affineMatrix*targetModel.nodeLevel3Spheres[i].toVector4D();
        targetModel.spheresCenter.append(affineVec);
    }
}

void drawGLWidget::drawFootprintsOnTarget_slot(){
    calculateAffineTranslate();
    compBoundingSphere();

    rayDirection.clear();
    screenStart.clear();

    spacecraftParam SCParameter;
    for(int i=0; i<SCParameters.length(); i++){
        if(SCParameters.at(i).SCName==SCName){
            SCParameter=SCParameters.at(i);
        }
    }
    for(int i=0; i<SCParameter.getRegisteredInstNum(); i++){
        if(SCParameter.instParamsOnSpacecraft.at(i).getIsDrawFootprint()==GL_TRUE){
            instParam instParameter=SCParameter.instParamsOnSpacecraft.at(i);
            instParameter.getProjection(projection);
            MygluUnProject(instParameter.getInstrumentType(), modelView, projection, viewport);
            crossingDetection("Boresight", instParameter.getFootprintColorCode(), GL_FALSE);
        }
    }

    if(isAutoDrawFootprints==GL_FALSE)
        updateGL();
}

void drawGLWidget::setIsDrawFootprint_signal(QString instName, SpiceBoolean isdrawFootprint){
    spacecraftParam SCParameter;
    int i=0;
    for(i=0; i<SCParameters.length(); i++){
        if(SCParameters.at(i).SCName==SCName){
            SCParameter=SCParameters.at(i);
        }
    }
    SCParameter.setIsDrawFootprint(instName, isdrawFootprint);
    SCParameters[i]=SCParameter;
}

void drawGLWidget::sendPolygonID_slot(){
    emit getPolygonID_signal(polygonIDInObservedArea, shadowFlag);
}

void drawGLWidget::isDrawShadow_slot(GLboolean drawShadow){
    isDrawShadow=drawShadow;

    if(isDrawShadow!=GL_TRUE){
        targetModel.recordShadow();
        targetModel.overwriteShadowOffData();
    }

    if(isDrawShadow==GL_TRUE&&drewShadow==GL_TRUE){
        isRestoredShadowData=GL_TRUE;
        isOnceRestore=GL_FALSE;
    }
    updateGL();
}

//Tomas Mollerの交差判定
//最初に球で交差判定をする。
//次に各ポリゴンごとに比較し、もっとも近いものを選ぶ
GLboolean drawGLWidget::crossingDetection(const QString instType, const QColor colorCode, GLboolean subPoint){
    GLboolean crossingDec=GL_FALSE;
    polygonIDInObservedArea.clear();
    shadowFlag.clear();
    QVector<GLboolean> isPolygonIDObserved;
    isPolygonIDObserved.resize(targetModel.target3DModel.faceNum);
    isPolygonIDObserved.fill(GL_FALSE);
    GLuint faceNum=targetModel.target3DModel.faceNum;
    //QVector4D footprintColor=QVector4D(colorCode.redF(), colorCode.greenF(), colorCode.blueF(), colorCode.alphaF());
    QVector4D footprintColor=QVector4D(0.0, 1.0, 0.0, colorCode.alphaF());
    GLboolean isFOV;
    if(instType=="FOV"){
        isFOV=1;
    }
    else{
        isFOV=0;
    }
    //Detection
    glBindBuffer(GL_ARRAY_BUFFER, targetModel.target3DModel.vboID[2]);
    GLfloat *clientPtrColor=(GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    GLint clienPtrColorSize=0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &clienPtrColorSize);

    GLfloat clip[16];
    GLfloat t;
    GLfloat frustum[6][4];

    clip[0]=modelView[0]*projection[0]+modelView[1]*projection[4]+modelView[2]*projection[8]+modelView[3]*projection[12];
    clip[1]=modelView[0]*projection[1]+modelView[1]*projection[5]+modelView[2]*projection[9]+modelView[3]*projection[13];
    clip[2]=modelView[0]*projection[2]+modelView[1]*projection[6]+modelView[2]*projection[10]+modelView[3]*projection[14];
    clip[3]=modelView[0]*projection[3]+modelView[1]*projection[7]+modelView[2]*projection[11]+modelView[3]*projection[15];

    clip[4]=modelView[4]*projection[0]+modelView[5]*projection[4]+modelView[6]*projection[8]+modelView[7]*projection[12];
    clip[5]=modelView[4]*projection[1]+modelView[5]*projection[5]+modelView[6]*projection[9]+modelView[7]*projection[13];
    clip[6]=modelView[4]*projection[2]+modelView[5]*projection[6]+modelView[6]*projection[10]+modelView[7]*projection[14];
    clip[7]=modelView[4]*projection[3]+modelView[5]*projection[7]+modelView[6]*projection[11]+modelView[7]*projection[15];

    clip[8]=modelView[8]*projection[0]+modelView[9]*projection[4]+modelView[10]*projection[8]+modelView[11]*projection[12];
    clip[9]=modelView[8]*projection[1]+modelView[9]*projection[5]+modelView[10]*projection[9]+modelView[11]*projection[13];
    clip[10]=modelView[8]*projection[2]+modelView[9]*projection[6]+modelView[10]*projection[10]+modelView[11]*projection[14];
    clip[11]=modelView[8]*projection[3]+modelView[9]*projection[7]+modelView[10]*projection[11]+modelView[11]*projection[15];

    clip[12]=modelView[12]*projection[0]+modelView[13]*projection[4]+modelView[14]*projection[8]+modelView[15]*projection[12];
    clip[13]=modelView[12]*projection[1]+modelView[13]*projection[5]+modelView[14]*projection[9]+modelView[15]*projection[13];
    clip[14]=modelView[12]*projection[2]+modelView[13]*projection[6]+modelView[14]*projection[10]+modelView[15]*projection[14];
    clip[15]=modelView[12]*projection[3]+modelView[13]*projection[7]+modelView[14]*projection[11]+modelView[15]*projection[15];

    /* Extract the numbers for the RIGHT plane */
    frustum[0][0]=clip[3]-clip[0];
    frustum[0][1]=clip[7]-clip[4];
    frustum[0][2]=clip[11]-clip[8];
    frustum[0][3]=clip[15]-clip[12];

    /* Normalize the result */
    t=sqrt(frustum[0][0]*frustum[0][0]+frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
    frustum[0][0]/=t;
    frustum[0][1]/=t;
    frustum[0][2]/=t;
    frustum[0][3]/=t;

    /* Extract the numbers for the LEFT plane */
    frustum[1][0]=clip[3]+clip[0];
    frustum[1][1]=clip[7]+clip[4];
    frustum[1][2]=clip[11]+clip[8];
    frustum[1][3]=clip[15]+clip[12];

    /* Normalize the result */
    t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
    frustum[1][0] /= t;
    frustum[1][1] /= t;
    frustum[1][2] /= t;
    frustum[1][3] /= t;

    /* Extract the BOTTOM plane */
    frustum[2][0] = clip[ 3] + clip[ 1];
    frustum[2][1] = clip[ 7] + clip[ 5];
    frustum[2][2] = clip[11] + clip[ 9];
    frustum[2][3] = clip[15] + clip[13];

    /* Normalize the result */
    t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
    frustum[2][0] /= t;
    frustum[2][1] /= t;
    frustum[2][2] /= t;
    frustum[2][3] /= t;

    /* Extract the TOP plane */
    frustum[3][0] = clip[ 3] - clip[ 1];
    frustum[3][1] = clip[ 7] - clip[ 5];
    frustum[3][2] = clip[11] - clip[ 9];
    frustum[3][3] = clip[15] - clip[13];

    /* Normalize the result */
    t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
    frustum[3][0] /= t;
    frustum[3][1] /= t;
    frustum[3][2] /= t;
    frustum[3][3] /= t;

    /* Extract the FAR plane */
    frustum[4][0] = clip[ 3] - clip[ 2];
    frustum[4][1] = clip[ 7] - clip[ 6];
    frustum[4][2] = clip[11] - clip[10];
    frustum[4][3] = clip[15] - clip[14];

    /* Normalize the result */
    t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
    frustum[4][0] /= t;
    frustum[4][1] /= t;
    frustum[4][2] /= t;
    frustum[4][3] /= t;

    /* Extract the NEAR plane */
    frustum[5][0] = clip[ 3] + clip[ 2];
    frustum[5][1] = clip[ 7] + clip[ 6];
    frustum[5][2] = clip[11] + clip[10];
    frustum[5][3] = clip[15] + clip[14];

    /* Normalize the result */
    t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
    frustum[5][0]/=t;
    frustum[5][1]/=t;
    frustum[5][2]/=t;
    frustum[5][3]/=t;

    // デバイスメモリを確保しつつデータをコピー
    GLint vertexNum=translatedVertex.length();
    //cout<<"verrexNum:"<<vertexNum<<endl;
    //QVector4D lightPosVec=QVector4D(lightpos[0], lightpos[1], lightpos[2], 1.0);
    cl::Buffer buffer1(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());
    cl::Buffer buffer2(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLint), &vertexNum);
    cl::Buffer buffer3(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*screenStart.length(), screenStart.data());
    cl::Buffer buffer4(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*rayDirection.length(), rayDirection.data());
    cl::Buffer buffer5(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*targetModel.spheresCenter.length(), targetModel.spheresCenter.data());
    cl::Buffer buffer6(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D), &footprintColor);
    cl::Buffer buffer7(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLboolean), &isFOV);
    cl::Buffer buffer8(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLboolean), &subPoint);
    cl::Buffer buffer9(context_ParallelCollision, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLfloat)*(6*4), &frustum[0]);
    cl::Buffer buffer10(context_ParallelCollision, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, (GLuint)clienPtrColorSize, clientPtrColor);
    cl::Buffer buffer11(context_ParallelCollision, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, sizeof(GLint), &subPolygonID);
    cl::Buffer buffer12(context_ParallelCollision, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, sizeof(GLboolean)*isPolygonIDObserved.length(), isPolygonIDObserved.data());

    /**/
    // カーネルの引数をセット
    //ComputeRayDirection
    q_ParallelCollision=cl::CommandQueue(context_ParallelCollision, stdDevice[0]);
    q_ParallelCollision.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());
    q_ParallelCollision.enqueueWriteBuffer(buffer2, CL_TRUE, 0, sizeof(GLint), &vertexNum);
    q_ParallelCollision.enqueueWriteBuffer(buffer3, CL_TRUE, 0, sizeof(QVector4D)*screenStart.length(), screenStart.data());
    q_ParallelCollision.enqueueWriteBuffer(buffer4, CL_TRUE, 0, sizeof(QVector4D)*rayDirection.length(), rayDirection.data());
    q_ParallelCollision.enqueueWriteBuffer(buffer5, CL_TRUE, 0, sizeof(QVector4D)*targetModel.spheresCenter.length(), targetModel.spheresCenter.data());
    q_ParallelCollision.enqueueWriteBuffer(buffer6, CL_TRUE, 0, sizeof(QVector4D), &footprintColor);
    q_ParallelCollision.enqueueWriteBuffer(buffer7, CL_TRUE, 0, sizeof(GLboolean), &isFOV);
    q_ParallelCollision.enqueueWriteBuffer(buffer8, CL_TRUE, 0, sizeof(GLboolean), &subPoint);
    q_ParallelCollision.enqueueWriteBuffer(buffer9, CL_TRUE, 0, sizeof(GLfloat)*(6*4), &frustum[0]);
    q_ParallelCollision.enqueueWriteBuffer(buffer10, CL_TRUE, 0, (GLuint)clienPtrColorSize, clientPtrColor);
    q_ParallelCollision.enqueueWriteBuffer(buffer11, CL_TRUE, 0, sizeof(GLint), &subPolygonID);
    q_ParallelCollision.enqueueWriteBuffer(buffer12, CL_TRUE, 0, sizeof(GLboolean)*isPolygonIDObserved.length(), isPolygonIDObserved.data());

    kernel_ParallelCollision=cl::Kernel(program_ParallelCollision, "collisionDetection");

    kernel_ParallelCollision.setArg(0, buffer1);
    kernel_ParallelCollision.setArg(1, buffer2);
    kernel_ParallelCollision.setArg(2, buffer3);
    kernel_ParallelCollision.setArg(3, buffer4);
    kernel_ParallelCollision.setArg(4, buffer5);
    kernel_ParallelCollision.setArg(5, buffer6);
    kernel_ParallelCollision.setArg(6, buffer7);
    kernel_ParallelCollision.setArg(7, buffer8);
    kernel_ParallelCollision.setArg(8, buffer9);
    kernel_ParallelCollision.setArg(9, buffer10);
    kernel_ParallelCollision.setArg(10, buffer11);
    kernel_ParallelCollision.setArg(11, buffer12);

    if(instType=="FOV"){
        cl::NDRange global(faceNum, 1, 1);
        q_ParallelCollision.enqueueNDRangeKernel(kernel_ParallelCollision, cl::NullRange, global);

    }
    else{
        cl::NDRange global(screenStart.length(), 1, 1);
        q_ParallelCollision.enqueueNDRangeKernel(kernel_ParallelCollision, cl::NullRange, global);
    }

    if(subPoint==GL_FALSE){
        q_ParallelCollision.enqueueReadBuffer(buffer10, CL_TRUE, 0, clienPtrColorSize, clientPtrColor);

    }
    else{
        q_ParallelCollision.enqueueReadBuffer(buffer11, CL_TRUE, 0, sizeof(GLint), &subPolygonID);
    }
    q_ParallelCollision.finish();
    q_ParallelCollision.flush();

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return crossingDec;
}

void drawGLWidget::drawShadow(){
    QVector4D lightPos;
    GLdouble placeX, placeY, placeZ;
    GLint minPlace;
    QVector4D affineVec;

    if(lightpos[0]!=0.0){
        placeX=(GLint)log10(abs(lightpos[0]))+1;
    }
    else{
        placeX=0;
    }

    if(lightpos[1]!=0.0){
        placeY=(GLint)log10(abs(lightpos[1]))+1;
    }
    else{
        placeY=0;
    }

    if(lightpos[2]!=0.0){
        placeZ=(GLint)log10(abs(lightpos[2]))+1;
    }
    else{
        placeZ=0;
    }

    minPlace=placeX;
    if(minPlace>placeY){
        minPlace=placeY;
    }
    if(minPlace>placeZ){
        minPlace=placeZ;
    }

    minPlace-=1;
    lightPos.setX(lightpos[0]/pow(10, minPlace));
    lightPos.setY(lightpos[1]/pow(10, minPlace));
    lightPos.setZ(lightpos[2]/pow(10, minPlace));
    lightPos.setW(0);

    GLint vertexNum=targetModel.target3DModel.vertexNum;
//    GLuint faceNum=targetModel.target3DModel.faceNum;

    glBindBuffer(GL_ARRAY_BUFFER, targetModel.target3DModel.vboID[2]);
    GLfloat *clientPtrColor=(GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    GLint clienPtrColorSize=0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &clienPtrColorSize);

    // デバイスメモリを確保しつつデータをコピー
    cl::Buffer buffer1(context_DrawShadows, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());
    cl::Buffer buffer2(context_DrawShadows, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLuint), &vertexNum);
    cl::Buffer buffer3(context_DrawShadows, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*targetModel.spheresCenter.length(), targetModel.spheresCenter.data());
    cl::Buffer buffer4(context_DrawShadows, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D), &lightPos);
    cl::Buffer buffer5(context_DrawShadows, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*vertexNum, clientPtrColor);

    // カーネルの引数をセット
    q_DrawShadows=cl::CommandQueue(context_DrawShadows, stdDevice[0]);
    q_DrawShadows.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());
    q_DrawShadows.enqueueWriteBuffer(buffer2, CL_TRUE, 0, sizeof(GLuint), &vertexNum);
    q_DrawShadows.enqueueWriteBuffer(buffer3, CL_TRUE, 0, sizeof(QVector4D)*targetModel.spheresCenter.length(), targetModel.spheresCenter.data());
    q_DrawShadows.enqueueWriteBuffer(buffer4, CL_TRUE, 0, sizeof(QVector4D), &lightPos);
    q_DrawShadows.enqueueWriteBuffer(buffer5, CL_TRUE, 0, sizeof(QVector4D)*vertexNum, clientPtrColor);

    kernel_DrawShadows=cl::Kernel(program_DrawShadows, "compShadowing");

    kernel_DrawShadows.setArg(0, buffer1);
    kernel_DrawShadows.setArg(1, buffer2);
    kernel_DrawShadows.setArg(2, buffer3);
    kernel_DrawShadows.setArg(3, buffer4);
    kernel_DrawShadows.setArg(4, buffer5);

    //Draw shadows
    // コマンドキューの作成
    cl::NDRange global(1, 1, 1);

    q_DrawShadows.enqueueNDRangeKernel(kernel_DrawShadows, cl::NDRange(0), cl::NDRange(vertexNum), global);
    q_DrawShadows.enqueueReadBuffer(buffer5, CL_TRUE, 0, (GLuint)clienPtrColorSize, clientPtrColor);
    //q_DrawShadows.enqueueReadBuffer(buffer5, CL_TRUE, 0, sizeof(QVector4D)*vertexNum, clientPtrColor);


    q_DrawShadows.flush();
    q_DrawShadows.finish();
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

QMatrix4x4 drawGLWidget::calculateMatrixInv(const GLfloat* mat){
    QMatrix4x4 matrix(mat);
    return matrix.transposed().inverted();
}

GLboolean drawGLWidget::MygluUnProject(const QString instType, const GLfloat* const modelView, const GLfloat* const projection, const GLint* const viewport)
{
    QVector<QVector4D> rayDir, start;
    QMatrix4x4 modelInv, projectionInv;
    QMatrix4x4 modelInvXprojInv;

    WandH.clear();
    if(instType=="FOV"){
        WandH.resize(FOVArea.length());
        WandH=FOVArea;
    }
    else if(instType=="Boresight"){
        WandH.resize(boresightArea.length());
        WandH=boresightArea;
    }
    else if(instType=="SubPoint"){
        WandH.append(QVector2D(wandHWidth, wandHHeight));
    }
    projectionInv=calculateMatrixInv(projection);
    modelInv=calculateMatrixInv(modelView);
    modelInvXprojInv=modelInv*projectionInv;
    GLint viewport2D[]={viewport[2], viewport[3]};

    rayDir.resize(WandH.length());
    start.resize(WandH.length());

    // デバイスメモリを確保しつつデータをコピー
    cl::Buffer buffer1(context_CompRayDir, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*WandH.length(), WandH.data());
    cl::Buffer buffer2(context_CompRayDir, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QMatrix4x4), &modelInvXprojInv);
    cl::Buffer buffer3(context_CompRayDir, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(unsigned int)*2, viewport2D);
    cl::Buffer buffer4(context_CompRayDir, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*rayDir.length(), rayDir.data());
    cl::Buffer buffer5(context_CompRayDir, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*start.length(), start.data());

    // カーネルの引数をセット
    //ComputeRayDirection
    q_CompRayDir=cl::CommandQueue(context_CompRayDir, stdDevice[0]);
    q_CompRayDir.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*WandH.length(), WandH.data());
    q_CompRayDir.enqueueWriteBuffer(buffer2, CL_TRUE, 0, sizeof(QMatrix4x4), &modelInvXprojInv);
    q_CompRayDir.enqueueWriteBuffer(buffer3, CL_TRUE, 0, sizeof(unsigned int)*2, viewport2D);

    kernel_CompRayDir=cl::Kernel(program_CompRayDir, "compRayDirection");

    kernel_CompRayDir.setArg(0, buffer1);
    kernel_CompRayDir.setArg(1, buffer2);
    kernel_CompRayDir.setArg(2, buffer3);
    kernel_CompRayDir.setArg(3, buffer4);
    kernel_CompRayDir.setArg(4, buffer5);

    //Detection
    // コマンドキューの作成
    cl::NDRange global(WandH.length(), 1, 1);
    cl_int execute=q_CompRayDir.enqueueNDRangeKernel(kernel_CompRayDir, cl::NullRange, global);

    q_CompRayDir.flush();
    q_CompRayDir.finish();

    q_CompRayDir.enqueueReadBuffer(buffer4, CL_TRUE, 0, sizeof(QVector4D)*rayDir.length(), rayDir.data());
    q_CompRayDir.enqueueReadBuffer(buffer5, CL_TRUE, 0, sizeof(QVector4D)*start.length(), start.data());

    rayDirection=rayDir;
    screenStart=start;

    return execute;
}

void drawGLWidget::calculateAffineTranslate(){
    //printf("calculateAffineTranslate\n");
    GLuint vertexNum=targetModel.target3DModel.vertexNum;
    GLuint faceNum=targetModel.target3DModel.faceNum;
    translatedVertex.clear();
    translatedVertex.resize(vertexNum);

    glBindBuffer(GL_ARRAY_BUFFER, targetModel.target3DModel.vboID[0]);
//    GLfloat *clientPtr=(GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    GLint clienPtrSize=0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &clienPtrSize);

    cl::Buffer buffer1(context_AffineTranslate, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*vertexNum, targetModel.target3DModel.verticesData.data());
    cl::Buffer buffer2(context_AffineTranslate, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QMatrix4x4), &affineMatrix);
    cl::Buffer buffer3(context_AffineTranslate, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());

    // カーネルの引数をセット
    //ComputeRayDirection
    q_AffineTranslate=cl::CommandQueue(context_AffineTranslate, stdDevice[0]);
    q_AffineTranslate.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*vertexNum, targetModel.target3DModel.verticesData.data());
    q_AffineTranslate.enqueueWriteBuffer(buffer2, CL_TRUE, 0, sizeof(QMatrix4x4), &affineMatrix);
    q_AffineTranslate.enqueueWriteBuffer(buffer3, CL_TRUE, 0, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());

    kernel_AffineTranslate=cl::Kernel(program_AffineTranslate, "AffineTranslate");

    kernel_AffineTranslate.setArg(0, buffer1);
    kernel_AffineTranslate.setArg(1, buffer2);
    kernel_AffineTranslate.setArg(2, buffer3);

    // コマンドキューの作成
    cl::NDRange global(faceNum, 1, 1);
    q_AffineTranslate.enqueueNDRangeKernel(kernel_AffineTranslate, cl::NullRange, global);

    q_AffineTranslate.enqueueReadBuffer(buffer3, CL_TRUE, 0, sizeof(QVector4D)*translatedVertex.length(), translatedVertex.data());

    q_AffineTranslate.finish();
    q_AffineTranslate.flush();

    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void drawGLWidget::compSphereMember(){
    QVector<QVector4D> spheres;
    QVector<GLboolean> isInPolygons;
    spheres.clear();
    GLuint vertexNum=targetModel.target3DModel.vertexNum;
    GLuint sphereNum=targetModel.nodeLevel3Spheres.length();

    isInPolygons.resize(targetModel.nodeLevel3Spheres.length());
    isInPolygons.fill(GL_FALSE);

    for(int i=0; i<targetModel.nodeLevel3Spheres.length(); i++){
        spheres.append(targetModel.nodeLevel3Spheres[i].toVector4D());
    }
    glBindBuffer(GL_ARRAY_BUFFER, targetModel.target3DModel.vboID[0]);
//    GLfloat *clientPtr=(GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    GLint clienPtrSize=0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &clienPtrSize);

    cl::Buffer buffer1(context_CompShpereMember, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*spheres.length(), spheres.data());
    cl::Buffer buffer2(context_CompShpereMember, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLuint), &sphereNum);
    cl::Buffer buffer3(context_CompShpereMember, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, sizeof(QVector4D)*targetModel.target3DModel.verticesData.length(), targetModel.target3DModel.verticesData.data());
    cl::Buffer buffer4(context_CompShpereMember, CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(GLboolean)*isInPolygons.length(), isInPolygons.data());

    // カーネルの引数をセット
    //ComputeRayDirection
    q_CompShpereMember=cl::CommandQueue(context_CompShpereMember, stdDevice[0]);
    q_CompShpereMember.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*spheres.length(), spheres.data());
    q_CompShpereMember.enqueueWriteBuffer(buffer2, CL_TRUE, 0, sizeof(GLuint), &sphereNum);
    q_CompShpereMember.enqueueWriteBuffer(buffer1, CL_TRUE, 0, sizeof(QVector4D)*targetModel.target3DModel.verticesData.length(), targetModel.target3DModel.verticesData.data());

    kernel_CompShpereMember=cl::Kernel(program_CompShpereMember, "AffineTranslate");

    kernel_CompShpereMember.setArg(0, buffer1);
    kernel_CompShpereMember.setArg(1, buffer2);
    kernel_CompShpereMember.setArg(2, buffer1);
    kernel_CompShpereMember.setArg(3, buffer2);
    // コマンドキューの作成
    cl::NDRange global(vertexNum, 1, 1);
    q_CompShpereMember.enqueueNDRangeKernel(kernel_CompShpereMember, cl::NullRange, global);
    q_CompShpereMember.enqueueReadBuffer(buffer3, CL_TRUE, 0, sizeof(QVector4D)*targetModel.target3DModel.verticesData.length(), targetModel.target3DModel.verticesData.data());
    q_CompShpereMember.enqueueReadBuffer(buffer4, CL_TRUE, 0, sizeof(GLboolean)*isInPolygons.length(), isInPolygons.data());

    q_CompShpereMember.finish();
    q_CompShpereMember.flush();

    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

int drawGLWidget::initCL(){
    cl_int err=CL_SUCCESS;
    // プラットフォーム一覧を取得
    stdPlat=platforms.toStdVector();
    cl::Platform::get(&stdPlat);
    platforms=platforms.fromStdVector(stdPlat);
    if(platforms.size()==0){
        cout<<"No platform\n"<<endl;
        return EXIT_FAILURE;
    }

    //cout<<platforms.size()<<" platform(s) found \n";

    // 見つかったプラットフォームの情報を印字
   /* for(auto p : stdPlat){
        cout<<"Platform: " << p.getInfo<CL_PLATFORM_NAME>() << "\n";
        cout<<"Vendor: " << p.getInfo<CL_PLATFORM_VENDOR>() << "\n";
        cout <<"Version: " << p.getInfo<CL_PLATFORM_VERSION>() << "\n";
        cout <<"Profile: " << p.getInfo<CL_PLATFORM_PROFILE>() << "\n";
        cout << "\n";
    }*/

    // デバイス一覧を取得
    //cout<<"Devices"<<endl;
    stdPlat[0].getDevices(CL_DEVICE_TYPE_GPU, &stdDevice);
    devices=devices.fromStdVector(stdDevice);
    if(devices.size()==0){
        cerr << "No device.\n";
        return EXIT_FAILURE;
    }

    // 見つかったデバイスの情報を印字
    /*for(auto d: stdDevice){
        cout<<"Platform: " << d.getInfo<CL_DEVICE_NAME>() << "\n";
        cout<<"Vendor: " << d.getInfo<CL_DEVICE_VENDOR>() << "\n";
        cout <<"Version: " << d.getInfo<CL_DEVICE_VERSION>() << "\n";
        cout <<"Profile: " << d.getInfo<CL_DEVICE_PROFILE>() << "\n";
        cout << "\n";
    }*/

    // コンテキストの作成
    context_CompRayDir=cl::Context(stdDevice);
    QString kernelName=":/compRayDirection.cl";
    QFile file(kernelName);

    if(file.open(QIODevice::ReadOnly)){

    }
    else{
        cout<<"Can't open file"<<endl;
        exit(0);
    }
    QString kernelSoure=codec->toUnicode(file.readAll());

    //cout<<kernelSoure.toStdString()<<endl;
    cl::Program::Sources source_CompRayDir(1, make_pair(kernelSoure.toLocal8Bit().constData(), kernelSoure.toLocal8Bit().length()+1));

    program_CompRayDir=cl::Program(context_CompRayDir, source_CompRayDir);
    if(program_CompRayDir.build(stdDevice)!=CL_SUCCESS){
        cout<<" Error building: "<<program_CompRayDir.getBuildInfo<CL_PROGRAM_BUILD_LOG>(stdDevice[0])<<"\n";
        exit(1);
    }

    context_ParallelCollision=cl::Context(stdDevice);

    kernelName=":/parallelCollisionDetection.cl";
    QFile file2(kernelName);

    if(file2.open(QIODevice::ReadOnly)){

    }
    else{
        cout<<"parallelCollisionDetection.cl can't open file"<<endl;
        exit(0);
    }

    QString kernelSoure2=codec->toUnicode(file2.readAll());

    cl::Program::Sources source_ParallelCollision(1, make_pair(kernelSoure2.toLocal8Bit().constData(), kernelSoure2.toLocal8Bit().length()+1));

    program_ParallelCollision=cl::Program(context_ParallelCollision, source_ParallelCollision);
    //program_CompRayDir.build(stdDevice);
    if(program_ParallelCollision.build(stdDevice)!=CL_SUCCESS){
        cout<<"parallelCollisionDetection.cl Error building: "<<program_ParallelCollision.getBuildInfo<CL_PROGRAM_BUILD_LOG>(stdDevice[0])<<"\n";
                exit(1);
    }

    context_AffineTranslate=cl::Context(stdDevice);

    kernelName=":/AffineTranslate.cl";
    QFile file3(kernelName);

    if(file3.open(QIODevice::ReadOnly)){

    }
    else{
        cout<<"affineTranslate.cl Can't open file"<<endl;
        exit(0);
    }
    QString kernelSoure3=codec->toUnicode(file3.readAll());

    cl::Program::Sources source_AffineTranslate(1, make_pair(kernelSoure3.toLocal8Bit().constData(), kernelSoure3.toLocal8Bit().length()+1));

    program_AffineTranslate=cl::Program(context_AffineTranslate, source_AffineTranslate);
    if(program_AffineTranslate.build(stdDevice)!=CL_SUCCESS){
        cout<<" affineTranslate.cl Error building: "<<program_AffineTranslate.getBuildInfo<CL_PROGRAM_BUILD_LOG>(stdDevice[0])<<"\n";
                exit(1);
    }

    context_DrawShadows=cl::Context(stdDevice);

    kernelName=":/compShadowing.cl";
    QFile file4(kernelName);

    if(file4.open(QIODevice::ReadOnly)){

    }
    else{
        cout<<"compShadowing.cl Can't open file"<<endl;
        exit(0);
    }

    QString kernelSoure4=codec->toUnicode(file4.readAll());
    cl::Program::Sources source_DrawShadows(1, make_pair(kernelSoure4.toLocal8Bit().constData(), kernelSoure4.toLocal8Bit().length()+1));

    program_DrawShadows=cl::Program(context_DrawShadows, source_DrawShadows);
    //program_CompRayDir.build(stdDevice);
    if(program_DrawShadows.build(stdDevice)!=CL_SUCCESS){
        cout<<" compShadowing.cl Error building: "<<program_DrawShadows.getBuildInfo<CL_PROGRAM_BUILD_LOG>(stdDevice[0])<<"\n";
        exit(1);
    }

    return err;
}

void drawGLWidget::mapTexture_slot(QString image, QString observedTime){
    SpiceDouble et;

    utc2et_c(observedTime.toStdString().c_str(), &et);

    //観測時の探査状態を計算

    compElements.computePos(SCName, targetName, et, sun_pos, earth_pos, sc_pos, target_pos);
    compElements.computeAttitude(instId, targetFrame, et, bsight, up_inst, &r, &p, &y);

    compElements.computeBoresightAndUpInstfromInput(targetFrame, instId, r, p, y, compBoresight, up_instFromInput);

/*
    posSC[0]=sc_pos[0];
    posSC[1]=sc_pos[1];
    posSC[2]=sc_pos[2];
    bsightDir[0]=sc_pos[0]+bsight[0];
    bsightDir[1]=sc_pos[1]+bsight[1];
    bsightDir[2]=sc_pos[2]+bsight[2];
    upVec[0]=up_inst[0];
    upVec[1]=up_inst[1];
    upVec[2]=up_inst[2];*/

    posSC[0]=sc_pos[0];
    posSC[1]=sc_pos[1];
    posSC[2]=sc_pos[2];
    bsightDir[0]=sc_pos[0]+bsight[0];
    bsightDir[1]=sc_pos[1]+bsight[1];
    bsightDir[2]=sc_pos[2]+bsight[2];
    upVec[0]=up_instFromInput[0];
    upVec[1]=up_instFromInput[1];
    upVec[2]=up_instFromInput[2];

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslated(0.5, 0.5, 0.0);

    //gluLookAt(sc_pos[0], sc_pos[1], sc_pos[2], bsight[0], bsight[1], bsight[2], up_inst[0], up_inst[1], up_inst[2]);
    gluLookAt(posSC[0], posSC[1], posSC[2], bsightDir[0], bsightDir[1], bsightDir[2], upVec[0], upVec[1], upVec[2]);

    qDebug() << posSC[0] << " " << posSC[1] << " " << posSC[2] << " " << bsightDir[0] << " " << bsightDir[1] << " " << bsightDir[2] << " " << upVec[0] << " " << upVec[1] << " " << upVec[2];

    //qDebug() << "Map: "<< bsight[0] << " " << bsight[1] << " " << bsight[2];

    //qDebug() << SCParameters.at(0).FOVAngle;

    //gluPerspective(30, 1.0, 1.0, 100.0);

    //gluPerspective(SCParameters.at(0).FOVAngle);

    targetModel.bindMapImage(image);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawGLWidget::clearTexture_slot(){
    targetModel.setTexture(0);

    updateGL();
}

void drawGLWidget::draw_NIRS_LINE_slot(bool div){
        isLine = div;

          update();
}
