#pragma once

#include <juce_graphics/juce_graphics.h>

namespace bonsai::theme
{
    inline const juce::Colour background      { 0xff16191c };
    inline const juce::Colour panel           { 0xff23272c };
    inline const juce::Colour panelBorder     { 0xff0e1012 };
    inline const juce::Colour trackHeader     { 0xff2b3036 };
    inline const juce::Colour trackHeaderSelected { 0xff3b4652 };
    inline const juce::Colour selection       { 0xff6fb6ff };
    inline const juce::Colour danger          { 0xffff7b72 };
    inline const juce::Colour muteRed         { 0xffe5484d };
    inline const juce::Colour soloYellow      { 0xffe8c547 };
    inline const juce::Colour laneBackground  { 0xff1b1e22 };
    inline const juce::Colour clipBackground  { 0xff23413a };
    inline const juce::Colour clipBorder      { 0xff3f7a6a };
    inline const juce::Colour waveform        { 0xff8fd9bf };
    inline const juce::Colour mutedClipBackground { 0xff2a2e33 };   // muted tracks are greyed out
    inline const juce::Colour mutedClipBorder { 0xff4a5058 };
    inline const juce::Colour mutedWaveform   { 0xff6c747c };
    inline const juce::Colour centreLine      { 0x40ffffff };
    inline const juce::Colour rulerText       { 0xffb8c0c8 };
    inline const juce::Colour rulerTick       { 0xff5a636c };
    inline const juce::Colour playhead        { 0xfff2c14e };
    inline const juce::Colour text            { 0xffe6e9ec };
    inline const juce::Colour dimText         { 0xff8a939c };
    inline const juce::Colour monoBadge       { 0xff4a7bd0 };
    inline const juce::Colour stereoBadge     { 0xffc0703f };
}
