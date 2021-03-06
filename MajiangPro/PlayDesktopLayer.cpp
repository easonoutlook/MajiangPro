//
//  PlayDesktopScene.cpp
//  MaJiong
//
//  Created by HalloWorld on 12-12-28.
//
//

#include "PlayDesktopLayer.h"
#include "MaJiongSprite.h"
#include "Definition.h"
#include <dispatch/dispatch.h>
#include "GameOverSceneSG.h"

#define kMaxSeconds 20
#define kAddTimerPercentage 15

using namespace cocos2d;

PlayDesktopLayer::PlayDesktopLayer()
{
}
PlayDesktopLayer::~PlayDesktopLayer()
{
    progress->stopAllActions();
}


bool PlayDesktopLayer::init()
{
    if (!DesktopLayer::init()) {
        return false;
    }
    
    return true;
}

void PlayDesktopLayer::initializePlayer()
{
    //两人同玩一pad对战
    playerScore = 0;
    progress = CCProgressTimer::create(CCSprite::create("progressLine.png"));
    progress->setAnchorPoint(ccp(0, 0));
    progress->setType( kCCProgressTimerTypeBar );
    addChild(progress);
    progress->setReverseDirection(false);
    progress->setBarChangeRate(ccp(0, 1));
    progress->setMidpoint(ccp(0, 0));
    progress->setPosition(ccp(10, 85));
    restartProgress();
}

void PlayDesktopLayer::restartProgress()
{
    progress->stopAllActions();
    progress->setPercentage(0);
    CCProgressTo *pt = CCProgressTo::create(kMaxSeconds, 100);
    CCCallFunc *func = CCCallFunc::create(this, callfunc_selector(PlayDesktopLayer::callFunProgress));
    CCFiniteTimeAction *seq = CCSequence::create(pt,func,NULL);
    progress->runAction((CCActionInterval *)seq);
}

void PlayDesktopLayer::callFunProgress()
{
    //Game Over
    progress->setVisible(false);
    GameOver();
}

void PlayDesktopLayer::initializeMajiong()
{
    MajiongsArray = CCArray::createWithCapacity(72);
    MajiongsArray->retain();
    for (int i = 9; i > 0; i --) {
        for (int j = 0; j < 4; j ++) {
            char name[111];
            sprintf(name, "1%d.png", i);
            MaJiongSprite *mj = MaJiongSprite::MaJiongWithFile(name);
            mj->AddSelectedObserver(this);
            MajiongsArray->addObject(mj);
            
            
            sprintf(name, "3%d.png", i);
            MaJiongSprite *mj1 = MaJiongSprite::MaJiongWithFile(name);
            mj1->AddSelectedObserver(this);
            MajiongsArray->addObject(mj1);
        }
    }
    
    randMaJiang();
}

void PlayDesktopLayer::randMaJiang()
{
    srand(time(NULL));
    int count = MajiongsArray->count();
    //产生一个0~72的随机序列数组
    int randIndex[72] = {0};
    int temp[72] = {0};
    for (int i = 0; i < 72; i++) {
        temp[i] = i;
    }
    
    int l = 71;
    for (int i = 0; i < 72; i ++) {
        int t = rand() % (72 - i);
        randIndex[i] = temp[t];
        temp[t] = temp[l];
        l --;
    }
    
    /*/输出
     printf("\n{");
     for (int i = 0; i < 72; i ++) {
     printf("%d, ", randIndex[i]);
     }
     printf("}\n");
     //*/
    
    //打乱所有麻将位置
    for (int i = 0; i < kMaxX; i ++) {
        for (int j = 0; j < kMaxY; j ++) {
            MaJiongSprite *majiong = (MaJiongSprite *)MajiongsArray->objectAtIndex(randIndex[i * kMaxY + j]);
            int x = i+1;
            int y = j+1;
            majiong->setOriginCoord(x, y);
            if (majiong->isVisible()) {
                DesktopMap[x][y] = DesktopStateMaJiong;
            } else {
                DesktopMap[x][y] = DesktopStateNone;
            }
            MJWidth = majiong->boundingBox().size.width;
            MJHeight = majiong->boundingBox().size.height;
            CCSize winSize = CCDirector::sharedDirector()->getWinSize();
            if (fabs(winSize.width - 768) < 0.01) {
                //ipad
                majiong->setPosition(ccp(OriginRootPad.x + MJWidth * x, OriginRootPad.y + MJHeight * y));
            } else {
                majiong->setPosition(ccp(OriginRoot.x + MJWidth * x, OriginRoot.y + MJHeight * y));
            }
            if (getChildByTag(i * kMaxY + j) == NULL) {
                this->addChild(majiong, 0, i * kMaxY + j);
            }
            count --;
        }
    }
}


void PlayDesktopLayer::SelectMajiong(MaJiongSprite *mj)
{
    CocosDenshion::SimpleAudioEngine::sharedEngine()->playEffect("select.mp3");
    if (tempSelectMajiong && mj) {
        if (tempSelectMajiong->isEqualTo(mj) && tempSelectMajiong != mj) {//1 在这个条件里面判断两麻将是否能联通
            //前后选择的两个麻将花色与大小相同
            vector<CCPoint> *linkPath = link(tempSelectMajiong->OringCoord, mj->OringCoord);
            if (linkPath != NULL) {//如果有路径可以连通
                CocosDenshion::SimpleAudioEngine::sharedEngine()->playEffect("disappear.mp3");
                //1绘制路径
                MaJiongDrawLinkPath(linkPath);
                //2消去两麻将
                SetDesktopState(tempSelectMajiong->OringCoord, DesktopStateNone);
                tempSelectMajiong->Disappear();
                SetDesktopState(mj->OringCoord, DesktopStateNone);
                mj->Disappear();
                //3当前玩家加分
                playerScore += mj->getMJScore();
                addTimeToProgress();
                handdleTurnPlayer(NULL);
                //4释放数组
                tempSelectMajiong = NULL;
                delete linkPath;
                linkPath = NULL;
            } else {
                tempSelectMajiong->Diselect();
                tempSelectMajiong = mj;
            }
        } else {
            tempSelectMajiong->Diselect();
            tempSelectMajiong = mj;
        }
    } else tempSelectMajiong = mj;
}

CCPoint PlayDesktopLayer::PositionOfCoord(CCPoint p)
{
    CCPoint pr = ccp(OriginRoot.x + MJWidth * p.x, OriginRoot.y + MJHeight * p.y);
    return pr;
}

void PlayDesktopLayer::addTimeToProgress()
{
    progress->stopAllActions();
    int cuper = progress->getPercentage();
    progress->setPercentage((cuper <= kAddTimerPercentage) ? (0) : (cuper - kAddTimerPercentage));
    CCProgressTo *pt = CCProgressTo::create(kMaxSeconds * (100 - progress->getPercentage()) / 100, 100);
    CCCallFunc *func = CCCallFunc::create(this, callfunc_selector(PlayDesktopLayer::callFunProgress));
    CCFiniteTimeAction *seq = CCSequence::create(pt,func,NULL);
    progress->runAction((CCActionInterval *)seq);
}

void PlayDesktopLayer::handdleTurnPlayer(PlayerLayer *player)
{
    if (tempSelectMajiong) {
        tempSelectMajiong->Diselect();
        tempSelectMajiong = NULL;
    }
    
    bool hasVisible = false;
    CCObject *obj = NULL;
    CCARRAY_FOREACH(MajiongsArray, obj){
        MaJiongSprite *mj = (MaJiongSprite *)obj;
        if (mj->isVisible()) {
            hasVisible = true;
        }
    }
    if (!hasVisible) {   //还有显示的麻将直接返回,不执行后面的
        //游戏正常结束逻辑
        RestartGame();
        return ;
    }
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        std::vector<CCPoint> *path = thereIsLink();
        if (path == NULL) {
            //调整麻将位置
            printf("\nNeed to random MJ Position!");
            dispatch_async(dispatch_get_main_queue(), ^{
                randMaJiang();
            });
        } else {
            std::vector<CCPoint>::iterator it = path->begin();
            printf("\n(%f, %f)", (*it).x, (*it).y);
        }
    });
}

void PlayDesktopLayer::GameOver()
{
    GameLayerScene *gameover = GameOverSceneSG::createWithScore(playerScore);
    gameover->setPlayAgainCategary(DesktopLayerSG);
    CCDirector::sharedDirector()->replaceScene(gameover);
    CocosDenshion::SimpleAudioEngine::sharedEngine()->playEffect("gameover.mp3");
}


void PlayDesktopLayer::RestartGame()
{
    //1 显示所有麻将
    setAllMJVisible();
    //2 打乱位置
    randMaJiang();
    //3 刷新时间进度条，游戏再开
    restartProgress();
}

void PlayDesktopLayer::setAllMJVisible()
{
    for (int i = 0; i < MajiongsArray->count(); i ++) {
        MaJiongSprite *mj = (MaJiongSprite *)MajiongsArray->objectAtIndex(i);
        mj->setVisible(true);
        mj->Diselect();
    }
    
    for (int i = 1; i < kMatrixMaxX-1; i ++) {
        for (int j = 1; j < kMatrixMaxY-1; j ++) {
            DesktopMap[i][j] = DesktopStateMaJiong;
        }
    }
}

