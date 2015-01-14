//
//  dpSwitchingManager.h
//  dpHakoniwa_cameraUnit
//
//  Created by Ovis aries on 2015/01/09.
//
//

#ifndef __dpHakoniwa_cameraUnit__dpSwitchingManager__
#define __dpHakoniwa_cameraUnit__dpSwitchingManager__

#include "ofMain.h"
#include "sw_1010F_SerialController.h"
#include "ofxOsc.h"

#define DISPLAY_SHIMO_OKU 4
#define DISPLAY_KAMI_OKU 5
#define DISPLAY_SHIMO_TEMAE 6
#define DISPLAY_KAMI_TEMAE 7

#define CVSW_1 0
#define CVSW_2 1
#define CVSW_3 2
#define CVSW_4 3

enum hakoniwaType{
	HAKO_STRUGGLE,
	HAKO_FROZENICE,
	HAKO_PLINK_LASER,
	HAKO_PLINK_MAGNET,
	HAKO_PLINK_PRISM,
	HAKO_COLOROFWATER,
	HAKO_SERVOPENDULUM,
	HAKO_BLANK,
};

struct hakoniwaPresets{
public:
	hakoniwaPresets(){};
	~hakoniwaPresets(){};

	hakoniwaType	type;
	string			CVPreset;
	int				sourceCh;
};

class cvSlot{
public:
	bool			isEmpty = true;
	hakoniwaType	hakoType;
	int				sourceCh;
	vector<int>		targetDisplay;
	string			presetFile;
	int				matrixInputCh;
};

class dpSwitchingManager{
public:

	void setup();
	void update();
	void draw();

	void receiveOscMessage(ofxOscMessage &m);

	void SelectHakoniwa(hakoniwaType type, int slot);

	void enableDisplay(hakoniwaType type,int displayNum,bool newHako);
	void disableHakoniwa(hakoniwaType type);
	void disableDisplay(int displayNum);

	bool isSlave;

	hakoniwaPresets* getHakoniwaPreset(hakoniwaType type);

	vector<hakoniwaPresets*> hakoniwas;

	ofxOscSender senderToSlave;

	cvSlot mSlots[4];
	sw_1010F_SerialController matrixSW;
};

#endif /* defined(__dpHakoniwa_cameraUnit__dpSwitchingManager__) */