#pragma once
#include "Object.h"

class IntroScreen : public Object {
private:
	TimerUtil    timer;
	SoundChannel IntroChannel{};

	GLfloat      LogoSize        = 1.0;
	GLfloat      LogoOpacity     = 0.0;
	GLfloat      LogoPosition    = -0.3;
	GLfloat      AnimationSpeed  = 4.0;

	int          SceneNumber{}; 

public:
	void InputKey(KeyEvent& Event) {
		if (Event.Type == NORMAL_KEY_DOWN) {
			switch (Event.NormalKey) {
			case NK_ENTER:
				soundUtil.Stop(IntroChannel);
				scene.SwitchMode(START_MODE);
				break;

#ifdef ENABLE_DEV_EXIT
			case NK_ESCAPE:
				System.Exit();
				break;
#endif
			}
		}
	}

	void UpdateFunc(float FrameTime) {
		timer.Update(FrameTime);
		switch (SceneNumber) {
		case 0:
			if (timer.CheckMiliSec(1.0, 1, CHECK_AND_INTERPOLATE)) {
				soundUtil.Play(SYSRES.INTRO_SOUND, IntroChannel);
				++SceneNumber;
			}
			break;
			

		case 1:
			if (timer.MiliSec() < 2.5) {
				mathUtil.Lerp(LogoPosition, 0.0, AnimationSpeed, FrameTime);
				mathUtil.Lerp(LogoOpacity, 1.0, AnimationSpeed, FrameTime);
			}

			if (timer.CheckMiliSec(2.5, 1, CHECK_AND_RESUME)) {
				LogoOpacity -= FrameTime * 2;
				EX.ClampValue(LogoOpacity, 0.0, CLAMP_LESS);
			}

			if (timer.CheckMiliSec(4.0, 1, CHECK_AND_INTERPOLATE)) {
				LogoPosition = -0.3;
				++SceneNumber;
			}
			break;


		case 2:
			if (timer.MiliSec() < 2.5) {
				mathUtil.Lerp(LogoPosition, 0.0, AnimationSpeed, FrameTime);
				mathUtil.Lerp(LogoOpacity, 1.0, AnimationSpeed, FrameTime);
			}

			if (timer.CheckMiliSec(2.5, 1, CHECK_AND_RESUME)) {
				LogoOpacity -= FrameTime * 2;
				EX.ClampValue(LogoOpacity, 0.0, CLAMP_LESS);
			}

			if (timer.CheckMiliSec(4.0, 1, CHECK_AND_RESUME))
				scene.SwitchMode(START_MODE);
			
			break;
		}
	}

	void RenderFunc() {
		Begin(RENDER_TYPE_STATIC);
		transform.Move(MoveMatrix, 0.0, LogoPosition);

		switch (SceneNumber) {
		case 1:
			imageUtil.RenderImage(SYSRES.SDK_LOGO, LogoOpacity);
			break;

		case 2: case 3:
			imageUtil.RenderImage(SYSRES.FMOD_LOGO, LogoOpacity);
			break;
		}
	}
};