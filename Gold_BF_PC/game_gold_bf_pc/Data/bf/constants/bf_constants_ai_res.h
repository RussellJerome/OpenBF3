#pragma once
//Dumped from Corra Build
//TODO Dump it from the og r7 since I fucked it up the first time

class bf_constants_ai_res
{
public:
    float k_ai_min_weapon_switch_time_bf = 20.0f;
    float k_ai_max_weapon_switch_time_bf = 40.0f;

    float k_ai_bf_fly_minPredictTimeAttack = 0.1f;
    float k_ai_bf_fly_maxPredictTimeAttack = 0.12f;
    float k_ai_bf_fly_predictTimePatrol = 40.0f;
    float k_ai_bf_fly_maxEvadeTime = 8.0f;
    float k_ai_bf_fly_maxAttackTime = 35.0f;

    float k_ai_bf_fly_minTimeBetweenAttacks = 4.0f;
    float k_ai_bf_fly_maxTimeBetweenAttacks = 6.0f;
    float k_ai_bf_fly_minTimeBetweenEvading = 10.0f;

    float k_ai_bf_fly_evadeForwardMultiplier = 1.0f;
    float k_ai_bf_fly_evadeSideMultiplier = 10.5f;

    float k_ai_bf_fly_takeOffYHeightFlat = 1.0f;
    float k_ai_bf_fly_takeOffYHeightRaised = 250.0f;

    float k_ai_bf_fly_takeOffDirMultiplierGround = 35.0f;
    float k_ai_bf_fly_takeOffDirMultiplierHangar = 250.0f;

    float k_ai_bf_fly_takeOffSearchRotDelta = 45.0f;

    float k_ai_bf_fly_followPlayerBankHeight = 40.0f;
    float k_ai_bf_fly_followPlayerBankRadius = 20.0f;

    float k_ai_fly_postAttackDistX = 0.0f;
    float k_ai_fly_postAttackDistY = 60.0f;
    float k_ai_fly_postAttackDistZ = 400.0f;

    float k_ai_fly_postAttackMinTurnDist = 200.0f;

    float k_ai_bf_fly_changeTargetMinTime = 2.0f;

    float k_ai_bf_fly_amountFacingForHeadOnThreat = 0.707f;

    float k_ai_bf_fly_maxFireDistance = 800.0f;
    float k_ai_bf_fly_maxFireAngleShip = 45.0f;
    float k_ai_bf_fly_maxFireAngleRemote = 45.0f;
    float k_ai_bf_fly_maxFireAngleStrafingRun = 45.0f;

    float k_ai_bf_fly_strafingRunYOffset = 10.0f;
    float k_ai_bf_fly_strafingRunInterval = 5.0f;

    float k_ai_bf_fly_randomLandInterval = 10.0f;
    float k_ai_bf_fly_randomLandIntervalTransports = 5.0f;

    float k_ai_bf_fly_timeBetweenUpdateThreat = 0.75f;

    float k_ai_bf_fly_maxFireBullets = 700.0f;
    float k_ai_bf_fly_timeBetweenFireSecondary = 20.0f;
    float k_ai_bf_fly_timeBetweenDropBombs = 2.0f;

    float k_ai_bf_fly_firePrimaryPercentage = 0.75f;

    float k_ai_bf_fly_endAttackTooCloseDist = 25.0f;
    float k_ai_bf_fly_maxDistMissileEvade = 40.0f;

    float k_ai_bf_fly_attackDesiredMinDist = 100.0f;
    float k_ai_bf_fly_attackDesiredMaxDist = 150.0f;

    float k_ai_bf_fly_attackMaxTimeInRange = 7.5f;

    int k_ai_bf_fly_maxNumThreatsPerTarget = 9;

    float k_ai_bf_fly_minThreatLevelForResponse = 100000.0f;

    float k_ai_bf_fly_multiplierLandingDirectionAfter = 500.0f;

    float k_ai_bf_fly_minLandingDistMultGround = 35.0f;
    float k_ai_bf_fly_maxLandingDistMultGround = 135.0f;

    float k_ai_bf_fly_minLandingDistMultHangar = 35.0f;
    float k_ai_bf_fly_maxLandingDistMultHangar = 70.0f;

    float k_ai_bf_fly_approachYLandingFromAboveGround = 48.0f;
    float k_ai_bf_fly_approachYLandingFromBelowGround = 48.0f;

    float k_ai_bf_fly_approachYLandingFromAboveHangar = 8.0f;
    float k_ai_bf_fly_approachYLandingFromBelowHangar = 8.0f;

    float k_ai_bf_fly_landingPitchMultiplier = 4.5f;
    float k_ai_bf_fly_landingYawMultiplier = 3.0f;

    float k_ai_bf_fly_minAIHeightMultiplier = 1.0f;

    float k_ai_bf_fly_abortLandingAfter = 5.0f;

    int k_ai_bf_fly_maxSquadronSize = 2;

    float k_ai_bf_fly_distBehindLeader = -25.0f;
    float k_ai_bf_fly_distSideOfLeader = 30.0f;

    float k_ai_bf_fly_catchUpMPS = 1.25f;

    float k_ai_bf_fly_minFollowTime = 20.0f;
    float k_ai_bf_fly_minFollowLocked = 15.0f;

    float k_ai_bf_fly_minDotLockOnToLeader = 0.5f;
    float k_ai_bf_fly_maxDistLockOnToLeader = 225.0f;

    float k_ai_bf_fly_waitForPassengerTime = 30.0f;

    float k_ai_bf_fly_maxWaitWithHumanPassenger = 5.0f;
    float k_ai_bf_fly_maxWaitNoPassengerVehicle = 3.0f;

    float k_ai_bf_fly_playerThreatMultiplier = 0.05f;

    float k_ai_min_reconsider_cloaking_bf = 50.0f;
    float k_ai_max_reconsider_cloaking_bf = 70.0f;

    float k_ai_cloak_target_timeout_bf = 3.0f;

    float k_ai_kraytDragonAttack2Distance = 20.0f;
    float k_rancorAttackDistance = 6.0f;
};