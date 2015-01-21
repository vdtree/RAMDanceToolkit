//
//  dpScoreMasterHakoniwa.cpp
//  dpScore
//
//  Created by YoshitoONISHI on 1/18/15.
//
//

#include "dpScoreMasterHakoniwa.h"
#include "ofxUI.h"

DP_SCORE_NAMESPACE_BEGIN

const ofColor MH::kBackgroundColor{255, 20};
const ofColor MH::kTextColor{255, 200};
const ofColor MH::kTextColorDark{255, 100};

const int MH::kValvePins[kNumValvePins]{2, 3, 4, 5, 6, 7};
const int MH::kPumpPins[kNumPumpPins]{8, 9};
const int MH::kPumpPinForward{kPumpPins[0]};
const int MH::kPumpPinBack{kPumpPins[1]};

const float MH::kPumpOpenDur[MH::kNumPumpPins]{8.0f, 5.0f};
const float MH::kPumpCloseDur[MH::kNumPumpPins]{2.0f, 5.0f};

const string MH::kHostNameMasterHakoniwa{"192.168.20.60"};
const int MH::kPortNumberMasterHakoniwa {8528};
const string MH::kHostNameCameraUnit{"192.168.20.5"};
const int MH::kPortNumberCameraUnit{12400};

const string MH::kOscAddrRamSetScene{"/ram/set_scene"};

const string MH::kXmlSettingsPath{"master_hakoniwa_settings.xml"};

#pragma mark ___________________________________________________________________
void MasterHakoniwa::Valve::update(MasterHakoniwa* mh)
{
    const float t{ofGetElapsedTimef()};
    
    if (!prevOpen && open) {
        openedTime = t;
        nOpend++;
    }
    else if (t - openedTime >= mh->mValveOpenDuration) {
        open = false;
        if (prevOpen && !open) {
            closedTime = t;
        }
    }
    prevOpen = open;
    
    mh->sendPin(pin, open);
}

#pragma mark ___________________________________________________________________
void MasterHakoniwa::Pump::update(MasterHakoniwa* mh)
{
    const float t{ofGetElapsedTimef()};
    
    if (!prevOpen && open) {
        openedTime = t;
    }
    else if (prevOpen && !open) {
        closedTime = t;
    }
    prevOpen = open;
    
    mh->sendPin(pin, open);
}

#pragma mark ___________________________________________________________________
bool MasterHakoniwa::Scene::isEnabled() const
{
    return window[WINDOW_0] || window[WINDOW_1];
}

#pragma mark ___________________________________________________________________
MasterHakoniwa& MasterHakoniwa::instance()
{
    static MasterHakoniwa instance;
    return instance;
}

void MasterHakoniwa::setupUI(ofxUITabBar* tabbar)
{
    const float sizeBig{40.f};
    const float sizeMid{25.f};
    const float w{kGuiWidth};
    const float h{ofGetHeight() * 0.5f + kMargin};
    const float lineH{12.f};
    tabbar->setWidth(w);
    tabbar->setColorBack(kBackgroundColor);
    tabbar->setPosition(kLineWidth, kTopOffset);
    tabbar->addToggle("[[[ Stop!!! ]]]", &mEmergencyStop, sizeBig, sizeBig);
    tabbar->addSpacer(w, 1.f);
    mEnableAllToggle = tabbar->addToggle("Enable All", false, sizeMid, sizeMid);
    tabbar->addToggle("Enable OSC RAM Dance Tool Kit", &mEnableOscOutRDTK);
    tabbar->addToggle("Enable OSC Master Hakoniwa", &mEnableOscOutMH);
    tabbar->addToggle("Enable CameraUnit", &mEnableCameraUnit);
    tabbar->addToggle("Enable MOTIONER", &mEnableMotioner);
    tabbar->addSpacer(w, 1.f);
    
    ofxUICanvas* sceneSelectTab{new ofxUICanvas()};
    sceneSelectTab->setWidth(w);
    sceneSelectTab->setHeight(h);
    sceneSelectTab->setColorBack(MH::kBackgroundColor);
    sceneSelectTab->setName("Select scene");
    sceneSelectTab->addLabel("Select scene", OFX_UI_FONT_SMALL);
    sceneSelectTab->addSpacer();
    vector<string> sceneNames;
    sceneNames.push_back("disable all");
    for (auto it : mScenes) {
        sceneNames.push_back(it.first);
    }
    sceneSelectTab->addRadio("Scenes", sceneNames);
    tabbar->addCanvas(sceneSelectTab);
    ofAddListener(sceneSelectTab->newGUIEvent, this, &MasterHakoniwa::guiEvent);
    
    tabbar->addSpacer(w, 1.f);
    tabbar->addLabel("Analyze method", OFX_UI_FONT_SMALL);
    vector<string> radioNames;
    radioNames.push_back("Mean");
    radioNames.push_back("Pixelate");
    tabbar->addRadio("Analyze", radioNames)->activateToggle("Mean");
    tabbar->addSpacer(w, 1.f);
    
    ofxUICanvas* meanTab{new ofxUICanvas};
    meanTab->setWidth(w);
    meanTab->setHeight(h);
    meanTab->setColorBack(MH::kBackgroundColor);
    meanTab->setName("Mean Settings");
    meanTab->addLabel("Mean Settings", OFX_UI_FONT_SMALL);
    meanTab->addSpacer();
    meanTab->addSlider("Limit", 0.1f, 100.f, &mAnalyzeMean.mMeanLimit, w - kMargin, lineH);
    meanTab->addSlider("Min time", 0.1f, 60.f * 3.f, &mAnalyzeMean.mMinSetSceneTime, w - kMargin, lineH);
    tabbar->addCanvas(meanTab);
    
    ofxUICanvas* pixelataTab{new ofxUICanvas};
    pixelataTab->setWidth(w);
    pixelataTab->setHeight(h);
    pixelataTab->setColorBack(MH::kBackgroundColor);
    pixelataTab->setName("Pixelate Settings");
    pixelataTab->addLabel("Pixelate Settings", OFX_UI_FONT_SMALL);
    pixelataTab->addSpacer();
    tabbar->addCanvas(pixelataTab);
    
    tabbar->addSpacer(w, 1.f);
    
    tabbar->addSlider("Valve open duration", 0.f, 1.f, &mValveOpenDuration, w, lineH);
    tabbar->addSpacer(w, 1.f);
    for (int i=0; i<kNumValvePins; i++) {
        tabbar->addToggle("Open valve " + ofToString(i), &mValves.at(i).open);
    }
    tabbar->addSpacer(w, 1.f);
    tabbar->addToggle("Open pump forward", &mPumps.at(0).open);
    tabbar->addToggle("Open pump back", &mPumps.at(1).open);
    tabbar->addSpacer(w, 1.f);
    
    tabbar->addLabel("MOTIONER method settings", OFX_UI_FONT_SMALL);
    
    ofAddListener(tabbar->newGUIEvent, this, &MasterHakoniwa::guiEvent);
}

void MasterHakoniwa::initialize()
{
    ofxXmlSettings xml;
    xml.load(kXmlSettingsPath);
    
    mValves.assign(kNumValvePins, Valve());
    for (int i=0; i<mValves.size(); i++) {
        mValves.at(i).pin = kValvePins[i];
    }
    
    mPumps.assign(kNumPumpPins, Pump());
    for (int i=0; i<mPumps.size(); i++) {
        mPumps.at(i).pin = kPumpPins[i];
    }
    
    mMasterHakoniwaOscServer.setup(kHostNameMasterHakoniwa,
                                   kPortNumberMasterHakoniwa);
    mCameraUnitOscServer.setup(kHostNameCameraUnit,
                               kPortNumberCameraUnit);
    turnOffAllPins();
    
    xml.pushTag("rdtk");
    xml.pushTag("phase", 0);
    vector<string> sceneNames;
    for (int j=0; j<xml.getNumTags("scene"); j++) {
        const string name{xml.getAttribute("scene", "name", "error", j)};
        mScenes[name] = Scene();
        sceneNames.push_back(name);
    }
    xml.popTag();
    xml.popTag();
    mUniqueScenes.setInitialList(sceneNames);
    
    xml.pushTag("score");
    const int numComplexity{xml.getNumTags("complexity")};
    mUniqueScores.assign(numComplexity, UniqueStringStack());
    for (int i=0; i<numComplexity; i++) {
        xml.pushTag("complexity", i);
        auto& stack = mUniqueScores.at(i);
        const int numScenes{xml.getNumTags("scene")};
        vector<string> sceneNames;
        for (int j=0; j<numScenes; j++) {
            const string name{xml.getAttribute("scene", "name", "error", j)};
            sceneNames.push_back(name);
            cout << name << endl;
        }
        stack.setInitialList(sceneNames);
        xml.popTag();
    }
    xml.popTag();
    
    ofAddListener(ofxMot::drawSkeletonEvent,
                  this,
                  &MasterHakoniwa::onDrawSkeleton);
}

void MasterHakoniwa::shutdown()
{
    ofRemoveListener(ofxMot::drawSkeletonEvent,
                     this,
                     &MasterHakoniwa::onDrawSkeleton);
    
    mEnableOscOutMH = true;
    turnOffAllPins();
}

void MasterHakoniwa::update()
{
    if (mEmergencyStop) {
        mEnableMotioner = false;
        mEnableCameraUnit = false;
        mEnableOscOutMH = false;
        mEnableOscOutRDTK = false;
        if (mEnableAllToggle) {
            mEnableAllToggle->setValue(false);
        }
        turnOffAllPins();
        turnOffAllScenes();
        mEmergencyStop = false;
    }
    
    if (ofGetFrameNum() % kUpdateFrames) return;
    
    for (int i=0; i<kNumValvePins; i++) {
        mValves[i].update(this);
    }
    
    for (int i=0; i<kNumPumpPins; i++) {
        mPumps[i].update(this);
    }
}

void MasterHakoniwa::updateCameraUnit(ofxEventMessage& m)
{
    if (m.getAddress() == kOscAddrCameraUnitMean) {
        if (m.getNumArgs() == ofVec4f::DIM) {
            ofVec4f mean;
            for (int i=0; i<ofVec4f::DIM; i++) {
                mean[i] = m.getArgAsInt32(i);
            }
            if (mEnableCameraUnit && mAnalyzeType == AnalyzeType::Mean) {
                mAnalyzeMean.update(mean);
            }
        }
    }
    else if (m.getAddress() == kOscAddrCameraUnitPixelateR) {
        if (mEnableCameraUnit)
            mAnalyzePixelate.updateColor(AnalyzePixelate::Color::R, m);
    }
    else if (m.getAddress() == kOscAddrCameraUnitPixelateG) {
        if (mEnableCameraUnit)
            mAnalyzePixelate.updateColor(AnalyzePixelate::Color::G, m);
    }
    else if (m.getAddress() == kOscAddrCameraUnitPixelateB) {
        if (mEnableCameraUnit)
            mAnalyzePixelate.updateColor(AnalyzePixelate::Color::B, m);
    }
}

void MasterHakoniwa::draw()
{
    const float x{kLineWidth + kGuiWidth + kMargin * 2.f + 3.f};
    const float y{400.f};
    mCamViewport.set(x,
                     y,
                     (ofGetWidth() - x) * 0.5f - kMargin * 2.f,
                     ofGetHeight() - y - kMargin * 2.f);
    
    mTextLeftCorner.set(mCamViewport.x + mCamViewport.width + kMargin * 2.f,
                        mCamViewport.y + kTextSpacing);
    
    const float t{ofGetElapsedTimef()};
    
    ofFill();
    ofSetColor(255, 0, 0, 20);
    ofRect(kMargin,
           kTopOffset - kMargin,
           ofGetWidth() - kMargin * 2.f,
           ofGetHeight() - kTopOffset);
    
    ofPushMatrix();
    alignedTranslate(kTextSpacing, kTopOffset);
    
    ofSetColor(kBackgroundColor);
    ofRect(0.f,
           0.f,
           kLineWidth - kTextSpacing - kMargin,
           ofGetHeight() - kTopOffset - kMargin * 2.f);
    
    ofSetColor(kTextColor);
    alignedTranslate(kMargin, kTextSpacing);
    ofDrawBitmapString("[motioner]", ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing);
    stringstream ss;
    ss << "running time: " << setprecision(1) << fixed
    << (mEnableMotioner ? ofGetElapsedTimef() - mEnabledTime : 0.f);
    mEnableMotioner ? ofSetColor(color::kMain) : ofSetColor(kTextColor);
    ofDrawBitmapString(ss.str(), ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing);
    alignedTranslate(0.f, kTextSpacing);
    
    ofSetColor(kTextColor);
    ofDrawBitmapString("[valves]", ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing);
    
    int i{0};
    for (auto& v : mValves) {
        v.open ? ofSetColor(color::kMain) : ofSetColor(kTextColor);
        stringstream ss;
        ss << i << " = " << (v.open ? "on " : "off")
        << setprecision(2) << fixed
        << " : " << (v.open ? t - v.openedTime : 0.f)
        << " / " << mValveOpenDuration
        << " | " << (!v.open ? t - v.closedTime : 0.f);
        ofDrawBitmapString(ss.str(), ofPoint::zero());
        alignedTranslate(0.f, kTextSpacing);
        i++;
    }
    
    ofSetColor(kTextColor);
    alignedTranslate(0.f, kTextSpacing);
    ofDrawBitmapString("[pumps]", ofPoint::zero());
    
    alignedTranslate(0.f, kTextSpacing);
    i = 0;
    for (auto& p : mPumps) {
        p.open ? ofSetColor(color::kMain) : ofSetColor(kTextColor);
        stringstream ss;
        ss << i << " = " << (p.open ? "on " : "off")
        << setprecision(1) << fixed
        << " : " << (p.open ? t - p.openedTime : 0.f)
        << " | " << (!p.open ? t - p.closedTime : 0.f);
        ofDrawBitmapString(ss.str(), ofPoint::zero());
        alignedTranslate(0.f, kTextSpacing);
        i++;
    }
    
    ofSetColor(kTextColor);
    alignedTranslate(0.f, kTextSpacing);
    ofDrawBitmapString("[scenes]", ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing);
    for (auto it : mScenes) {
        const string name{it.first};
        auto& s = it.second;
        
        auto findIt = mUniqueScenes.find(name);
        if (s.isEnabled()) ofSetColor(color::kMain);
        else if (findIt != mUniqueScenes.end()) ofSetColor(kTextColor);
        else ofSetColor(kTextColorDark);
        
        ofDrawBitmapString(name, ofPoint::zero());
        stringstream ss;
        ss << "= " << (s.isEnabled() ? "on " : "off") << " : ";
        for (int i=0; i<NUM_WINDOWS; i++) {
            ss << (int)s.window[i];
            if (i < NUM_WINDOWS - 1) ss << ", ";
        }
        ofDrawBitmapString(ss.str(), ofPoint(kTextSpacing * 15.f, 0.f));
        alignedTranslate(0.f, kTextSpacing);
    }
    alignedTranslate(0.f, kTextSpacing);
    
    ofSetColor(kTextColor);
    ss.str("");
    ss << "unique scenes: " << mUniqueScenes.size();
    ofDrawBitmapString(ss.str(), ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing * 2.f);

    ofDrawBitmapString("[score]", ofPoint::zero());
    for (int i=0; i<mUniqueScores.size(); i++) {
        auto& stack = mUniqueScores.at(i);
        alignedTranslate(0.f, kTextSpacing);
        
        ofDrawBitmapString("- complexity " + ofToString(i) + " -", ofPoint::zero());
        alignedTranslate(0.f, kTextSpacing);
        for (int j=0; j < stack.getInitialList().size(); j++) {
            const auto& s = stack.getInitialList().at(j);
            auto findIt = stack.find(s);
            if (s == mCurrentScore) ofSetColor(color::kMain);
            else if (findIt != stack.end()) ofSetColor(kTextColor);
            else ofSetColor(kTextColorDark);
            string ss{s};
            ofStringReplace(ss, "dp::score::Scene", "");
            ofDrawBitmapString(ss, ofPoint::zero());
            alignedTranslate(0.f, kTextSpacing);
        }
    }
    
    ofPopMatrix();
    
    

    ofPushMatrix();
    
    alignedTranslate(kLineWidth + kGuiWidth * 2.f + kMargin * 3.f, kTopOffset);
    ofSetColor(kBackgroundColor);
    alignedRect(0.f,
                0.f,
                ofGetWidth() - (kLineWidth + kGuiWidth * 2.f  + kMargin * 6.f),
                ofGetHeight() - (mCamViewport.height + kTopOffset + kMargin * 3.f + 1.f));
    
    alignedTranslate(kMargin, 0.f);
    ofSetColor(kTextColor);
    alignedTranslate(0.f, kTextSpacing);
    
    ofPushMatrix();
    alignedTranslate(0.f, kTextSpacing);
    mAnalyzeMean.draw();
    ofPopMatrix();
    
    ofPushMatrix();
    ofDrawBitmapString("[mean]", ofPoint::zero());
    alignedTranslate(200.f, kTextSpacing);
    ss.str("");
    ss
    << "data span : " << setprecision(3) << mAnalyzeMean.mLastUpdateSpan << endl
    << "scene span: " << setprecision(1) << t - mAnalyzeMean.mPrevSetSceneTime << endl
    << "total add : " << setprecision(3) << mAnalyzeMean.mTotalAddition.f << endl
    << "raw color : " << setprecision(0) << mAnalyzeMean.mMean << endl
    << "diff add  : " << setprecision(1) << mAnalyzeMean.mMeanAddtion;
    
    ofDrawBitmapString(ss.str(), ofPoint::zero());
    ofPopMatrix();
    
    alignedTranslate(0.f, kTextSpacing * 8.f);
    
    ofPushMatrix();
    ofSetColor(kTextColor);
    ofDrawBitmapString("[pixelate]", ofPoint::zero());
    alignedTranslate(0.f, kTextSpacing);
    mAnalyzePixelate.draw();
    ofPopMatrix();
    
    ofPopMatrix();
    
    
    ofSetColor(kBackgroundColor);
    ofRect(mCamViewport);
    ofPushMatrix();
    alignedTranslate(mCamViewport.width + kMargin, 0.f);
    ofRect(mCamViewport);
    ofPopMatrix();
    
    ofPushStyle();
    ofEnableDepthTest();
    mCam.begin(mCamViewport);
    alignedTranslate(0.f, -100.f);
    ofSetRectMode(OF_RECTMODE_CENTER);
    ofRotateX(90.f);
    ofSetColor(kBackgroundColor, 5);
    ofRect(ofPoint::zero(), 10000, 1000);
    mCam.end();
    ofPopStyle();
}

void MasterHakoniwa::turnOnValve(int index)
{
    if (!mEnableMotioner) return;
    
    if (index <0 || index >= mValves.size())
        ofxThrowExceptionf(ofxException, "valve index %d is out of range", index);
    mValves.at(index).open = true;
}

void MasterHakoniwa::turnOffAllPins()
{
    for (auto& v : mValves) {
        v.open = false;
        sendPin(v.pin, v.open);
    }
    for (auto& p : mPumps) {
        p.open = false;
        sendPin(p.pin, p.open);
    }
}

bool MasterHakoniwa::getIsOpeningValve(int index)
{
    if (index <0 || index >= mValves.size())
        ofxThrowExceptionf(ofxException, "valve index %d is out of range", index);
    return mValves.at(index).open;
}

void MasterHakoniwa::setUniqueScene(int sceneIndex, bool win0, bool win1)
{
    if (!getIsWindowOn(0)) win0 = true;
    if (!getIsWindowOn(1)) win1 = true;
    
    if (!win0 && !win1) win0 = win1 = true;
    
    const string sceneName{mUniqueScenes.get(sceneIndex)};
    
    sendSetScene(sceneName, win0, win1);
}

bool MasterHakoniwa::getIsWindowOn(int windowIndex) const
{
    if (windowIndex < 0 || windowIndex >= NUM_WINDOWS) {
        ofxThrowExceptionf(ofxException, "window index %d out of range", windowIndex);
    }
    
    bool on{false};
    for (auto it : mScenes) {
        if (it.second.window[windowIndex]) on = true;
    }
    return on;
}

void MasterHakoniwa::turnOffAllScenes()
{
    for (auto it : mScenes) {
        const string& name{it.first};
        const auto&s = it.second;
        sendSetScene(name, false, false);
    }
}

#pragma mark ___________________________________________________________________
void MasterHakoniwa::sendPin(int pin, bool open)
{
    ofxOscMessage m;
    m.setAddress("/dp/hakoniwa/colorOfWater/"+ofToString(pin));
    m.addIntArg((int)open);
    if (mEnableOscOutMH) mMasterHakoniwaOscServer.sendMessage(m);
    //if (open)
    //    cout << pin << "=" << open << endl;
}

void MasterHakoniwa::sendSetScene(const string& name, bool win0, bool win1)
{
    auto& scene = mScenes[name];
    if (scene.window[WINDOW_0] == win0 && scene.window[WINDOW_1] == win1) {
        return;
    }
    
    scene.window[WINDOW_0] = win0;
    scene.window[WINDOW_1] = win1;
    scene.dirty = true;
    
    for (auto& it : mScenes) {
        auto& s = it.second;
        const string& sName = it.first;
        
        if (it.first != name) {
            for (int i=0; i<NUM_WINDOWS; i++) {
                if (scene.window[i] && s.window[i]) {
                    it.second.window[i] = false;
                    it.second.dirty = true;
                }
            }
        }
        if (!s.dirty) continue;
        
        //stringstream ss;
        ofxOscMessage m;
        m.setAddress(kOscAddrRamSetScene);
        m.addStringArg(sName);
        m.addIntArg((int)s.isEnabled());
        
        //ss << boolalpha << sName << "=" << (s.isEnabled() ? "on " : "off") << " : ";
        
        for (int i=0; i<NUM_WINDOWS; i++) {
            m.addIntArg((int)s.window[i]);
            //ss << s.window[i] << ", ";
        }
        if (mEnableOscOutRDTK) mCameraUnitOscServer.sendMessage(m);
        //ofLogNotice() << ss.str();
    }
}

void MasterHakoniwa::onDrawSkeleton(ofxMotioner::EventArgs &e)
{
    mCam.begin(mCamViewport);
    ofEnableDepthTest();
    ofPushMatrix();
    ofPushStyle();
    alignedTranslate(0.f, -100.f + 8.f);
    
    ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
    
    auto& joints = e.skeleton->getJoints();
    
    ofNoFill();
    ofSetSphereResolution(2);
    
    for (size_t i=0; i<joints.size(); i++) {
        
        ofSetColor(dp::score::color::kDarkPinkHeavy);
        
        ofSetLineWidth(1.0f);
        auto& n = joints.at(i);
        n.transformGL();
        ofDrawSphere(8.f);
        n.restoreTransformGL();
        
        if (!n.getParent()) continue;
        
        ofSetColor(dp::score::color::kDarkPinkHeavy);
        ofLine(n.getGlobalPosition(), n.getParent()->getGlobalPosition());
    }
    
    ofPushMatrix();
    ofSetColor(dp::score::color::kMain);
    ofMultMatrix(joints.at(ofxMot::JOINT_HEAD).getGlobalTransformMatrix());
    ofDrawBitmapString(e.skeleton->getName(),
                       ofPoint(0.0f, joints.at(ofxMot::JOINT_HEAD).size));
    ofPopMatrix();
    
    ofPopStyle();
    ofPopMatrix();
    
    mCam.end();
}

void MasterHakoniwa::guiEvent(ofxUIEventArgs& e)
{
    const string& widgetName{e.widget->getName()};
    
    if (widgetName == "Enable") {
        mEnabledTime = ofGetElapsedTimef();
    }
    else if (widgetName == "Analyze") {
        auto* radio = static_cast<ofxUIRadio*>(e.widget);
        const auto& toggleName = radio->getActiveName();
        if (toggleName == "Mean") {
            mAnalyzeType = AnalyzeType::Mean;
            ofLogNotice() << "Cnahged analyze type to mean";
        }
        else if (toggleName == "Pixelate") {
            mAnalyzeType = AnalyzeType::Pixelate;
            ofLogNotice() << "Cnahged analyze type to pixelate";
        }
    }
    else if (widgetName == "Scenes") {
        auto* radio = static_cast<ofxUIRadio*>(e.widget);
        const auto& toggleName = radio->getActiveName();
        if (radio->getActive()->getValue()) {
            if (toggleName == "disable all") {
                turnOffAllScenes();
            }
            else {
                sendSetScene(toggleName, true, true);
            }
        }
    }
    else if (widgetName == "Enable All") {
        const bool t{static_cast<ofxUIToggle*>(e.widget)->getValue()};
        mEnableOscOutMH = t;
        mEnableOscOutRDTK = t;
        mEnableMotioner = t;
        mEnableCameraUnit = t;
    }
}

DP_SCORE_NAMESPACE_END