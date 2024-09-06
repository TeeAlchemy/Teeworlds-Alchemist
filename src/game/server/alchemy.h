#ifndef GAME_SERVER_ALCHEMY_H
#define GAME_SERVER_ALCHEMY_H

class IAlchemy
{
private:
    // blending (big then 0 is thermal, less then 0 is cold)
    // 0 is the gold
    short m_Attribute;

public:
    IAlchemy(/* args */);
    ~IAlchemy();

/*****EVENT*****/
public:
    virtual void Use() = 0;
    virtual void s() = 0;
};

#endif