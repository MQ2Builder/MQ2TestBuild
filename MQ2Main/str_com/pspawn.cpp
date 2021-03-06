#include <stdio.h>
#include "../MQ2Plugin.h"

#define KS(x)  printf("/* 0x%04x */ \n", &p->x)

main()
{
struct _SPAWNINFO *p = NULL;

KS(vtable);
KS(pPrev);
KS(pNext);
KS(Unknown0xc);
KS(Lastname);
KS(Unknown0x30);
KS(Y);
KS(X);
KS(Z);
KS(SpeedY);
KS(SpeedX);
KS(SpeedZ);
KS(SpeedRun);
KS(Heading);
KS(SpeedHeading);
KS(Unknown0x60);
KS(CameraAngle);
KS(SpawnID);
KS(Unknown0x70);
KS(Name);
KS(DisplayedName);
KS(IsABoat);
KS(Unknown0xf8);
KS(Mount);
KS(Unknown0x100);
KS(SpeedMultiplier);
KS(Unknown0x118);
KS(TimeStamp);
KS(Unknown0x12c);
KS(UnderWater);
KS(Unknown0x131);
KS(FeetWet);
KS(Unknown0x13e);
KS(Unknown0x13f);
KS(Type);
KS(Unknown0x141);
KS(BodyType);
KS(Unknown0x145);
KS(AvatarHeight);
KS(Unknown0x158);
KS(Suffix);
KS(LastTick);
KS(Unknown0x190);
KS(GuildStatus);
KS(Unknown0x198);
KS(InnateETA);
KS(GuildID);
KS(Unknown0x1c4);
KS(Light);
KS(Holding);
KS(Unknown0x1e6);
KS(SpellETA);
KS(CastingSpellID);
KS(CastingAnimation);
KS(Unknown0x1f4);
KS(pCastingClicky);
KS(CastingY);
KS(CastingX);
KS(Unknown0x218);
KS(Deity);
KS(Zone);
KS(Instance);
KS(FishingEvent);
KS(Unknown0x225);
KS(Buyer);
KS(RunSpeed);
KS(Unknown0x23c);
KS(Unknown0x26c);
KS(Sneak);
KS(Unknown0x271);
KS(PvPFlag);
KS(Unknown0x279);
KS(StandState);
KS(Unknown0x281);
KS(WalkSpeed);
KS(Unknown0x288);
KS(Trader);
KS(Unknown0x290);
KS(HideMode);
KS(Unknown0x299);
KS(PetID);
KS(Unknown0x304);
KS(LFG);
KS(Unknown0x30a);
KS(GM);
KS(AARank);
KS(Unknown0x310);
KS(HPMax);
KS(Unknown0x31c);
KS(Linkdead);
KS(Unknown0x321);
KS(Anon);
KS(Unknown0x330);
KS(FishingETA);
KS(Unknown0x338);
KS(pCharInfo_vtable2);
KS(Unknown0x3f8);
KS(AFK);
KS(Title);
KS(Unknown0x434);
KS(Level);
KS(Unknown0x447);
KS(MasterID);
KS(Unknown0x470);
KS(HPCurrent);
KS(Unknown0x484);
KS(WhoFollowing);
KS(pGroupAssistNPC);
KS(pRaidAssistNPC);
KS(pGroupMarkNPC);
KS(pRaidMarkNPC);
KS(pTargetOfTarget);
KS(Unknown0xddc);
KS(InNonPCRaceIllusion);
KS(Unknown0xe11);
KS(FaceStyle);
KS(HairColor);
KS(FacialHairColor);
KS(EyeColor1);
KS(EyeColor2);
KS(HairStyle);
KS(FacialHair);
KS(Unknown0xe21);
KS(Race);
KS(Class);
KS(Gender);
KS(ActorDef);
KS(Unknown0xe62);
KS(ArmorColor);
KS(Unknown0xe88);
KS(Equipment);
KS(Unknown0xf04);
KS(pcactorex);
KS(Unknown0xf40);
KS(FaceRelatedActorStruct);
KS(Unknown0xf48);
KS(Animation);
KS(Unknown0xfb8);
KS(Model);
KS(HideCorpse);
KS(Unknown0x10c8);
KS(InvitedToGroup);
KS(Unknown0x1109);
KS(vtable2);
KS(Unknown0x12e8);
KS(pSpawn);
KS(Levitate);
KS(Unknown0x12f1);
printf("/* 0x%04x */\n", sizeof(SPAWNINFO));
}
