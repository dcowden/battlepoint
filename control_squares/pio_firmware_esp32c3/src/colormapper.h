#pragma once

#include <Arduino.h>
#include "Rgb.h"
#include "Teams.h"
#define BRIGHTNESS 200
#
// Bridge between Team/TeamColor and actual RGB values for NeoPixel,
// plus fg/bg policy for the mini control point indicator.

class ColorMapper {
public:
    // Map TeamColor enum to real RGB values for NeoPixel.
    // Adjust these triplets as needed.
    static Rgb fromTeamColor(TeamColor c) {
        switch (c) {
            case TeamColor::COLOR_RED:
                return Rgb(BRIGHTNESS, 0, 0);
            case TeamColor::COLOR_BLUE:
                return Rgb(0, 0, BRIGHTNESS);
            case TeamColor::COLOR_YELLOW:
                return Rgb(BRIGHTNESS, BRIGHTNESS, 0);
            case TeamColor::COLOR_BLACK:
                return Rgb(0, 0, 0);
            case TeamColor::COLOR_GREEN:
                return Rgb(0, BRIGHTNESS, 0);                
            case TeamColor::COLOR_AQUA:
                // Neutral / third color; tweak as needed
                return Rgb(0, BRIGHTNESS, BRIGHTNESS);
            default:
                return Rgb(0, 0, 0);
        }
    }

    // Map Team to RGB via your existing getTeamColor()
    static Rgb fromTeam(Team t) {
        TeamColor tc = getTeamColor(t);
        return fromTeamColor(tc);
    }

    // Background color is always owner's color (or black/NOBODY)
    static Rgb backgroundFor(Team owner) {
        return fromTeam(owner);
    }

    // Foreground color rules:
    //
    // 1) If a team is actively capturing (capturing != NOBODY) → that team's color.
    //    - capturing == RED      → red fg
    //    - capturing == BLU      → blue fg
    //    - capturing == NEUTRAL  → neutral fg (third color)
    //
    // 2) If nobody is capturing but someone owns:
    //        RED owner      → BLU fg (oppositeTeam(RED))
    //        BLU owner      → RED fg (oppositeTeam(BLU))
    //        NEUTRAL owner  → NEUTRAL fg
    //        ALL owner      → ALL color (if you use that)
    //
    // 3) If nobody owns AND nobody is capturing → NEUTRAL color
    //
    static Rgb foregroundFor(Team owner, Team capturing) {
        // 1) Active capture → capturing team color
        if (capturing != Team::NOBODY) {
            return fromTeam(capturing);
        }

        // 2) No capture, but there is an owner
        switch (owner) {
            case Team::RED:
            case Team::BLU: {
                // Use your tested oppositeTeam() helper:
                Team attacker = oppositeTeam(owner); // RED <-> BLU, others -> NOBODY
                if (attacker != Team::NOBODY) {
                    return fromTeam(attacker);
                }
                // fall through if oppositeTeam gave NOBODY
                break;
            }

            case Team::NEUTRAL:
                // Neutral owner → use neutral/third color
                return fromTeam(Team::NEUTRAL);

            case Team::ALL:
                // If you want ALL to show as neutral/third color:
                return fromTeam(Team::ALL);

            case Team::NOBODY:
            default:
                break;
        }

        // 3) No owner, no capturing → neutral/ready color
        return fromTeam(Team::NEUTRAL);
    }
};
