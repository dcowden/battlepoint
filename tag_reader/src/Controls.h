#pragma once

void initInputs();
void updateInputs();
void registerLittleHitHandler(void (*callback)(void));
void registerBigHitHandler(void (*callback)(void));