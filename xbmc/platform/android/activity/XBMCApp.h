#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#include <math.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <map>

#include <android/native_activity.h>

#include "IActivityHandler.h"
#include "IInputHandler.h"

#include "platform/xbmc.h"
#include "platform/android/jni/Activity.h"
#include "platform/android/jni/BroadcastReceiver.h"
#include "platform/android/jni/AudioManager.h"
#include "platform/android/jni/View.h"
#include "platform/android/jni/AudioDeviceInfo.h"
#include "threads/Event.h"
#include "interfaces/IAnnouncer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#include "JNIMainActivity.h"

#include "guilib/Geometry.h"

// forward delares
class CJNIWakeLock;
class CAESinkAUDIOTRACK;
class CVariant;
class IInputDeviceCallbacks;
class IInputDeviceEventHandler;
class CVideoSyncAndroid;
typedef struct _JNIEnv JNIEnv;

struct androidIcon
{
  unsigned int width;
  unsigned int height;
  void *pixels;
};

struct androidPackage
{
  std::string packageName;
  std::string packageLabel;
  int icon;
};

class CActivityResultEvent : public CEvent
{
public:
  CActivityResultEvent(int requestcode)
    : m_requestcode(requestcode)
  {}
  int GetRequestCode() const { return m_requestcode; }
  int GetResultCode() const { return m_resultcode; }
  void SetResultCode(int resultcode) { m_resultcode = resultcode; }
  CJNIIntent GetResultData() const { return m_resultdata; }
  void SetResultData(const CJNIIntent &resultdata) { m_resultdata = resultdata; }

protected:
  int m_requestcode;
  CJNIIntent m_resultdata;
  int m_resultcode;
};

class CXBMCApp
    : public IActivityHandler
    , public CJNIMainActivity
    , public CJNIBroadcastReceiver
    , public CJNIAudioManagerAudioFocusChangeListener
    , public ANNOUNCEMENT::IAnnouncer
{
public:
  CXBMCApp(ANativeActivity *nativeActivity);
  virtual ~CXBMCApp();
  static CXBMCApp* get() { return m_xbmcappinstance; }

  // IAnnouncer IF
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  virtual void onReceive(CJNIIntent intent);
  virtual void onNewIntent(CJNIIntent intent);
  virtual void onActivityResult(int requestCode, int resultCode, CJNIIntent resultData);
  virtual void onVolumeChanged(int volume);
  virtual void onAudioFocusChange(int focusChange);
  virtual void doFrame(int64_t frameTimeNanos);
  virtual void onAudioDeviceAdded(CJNIAudioDeviceInfos devices);
  virtual void onAudioDeviceRemoved(CJNIAudioDeviceInfos devices);
  virtual void onVideoViewLost();

  // implementation of CJNIInputManagerInputDeviceListener
  void onInputDeviceAdded(int deviceId) override;
  void onInputDeviceChanged(int deviceId) override;
  void onInputDeviceRemoved(int deviceId) override;

  bool isValid() { return m_activity != NULL; }

  void onStart();
  void onResume();
  void onPause();
  void onStop();
  void onDestroy();

  void onSaveState(void **data, size_t *size);
  void onConfigurationChanged();
  void onLowMemory();

  void onCreateWindow(ANativeWindow* window);
  void onResizeWindow();
  void onDestroyWindow();
  void onGainFocus();
  void onLostFocus();

  void Initialize();
  void Deinitialize();

  static const ANativeWindow** GetNativeWindow(int timeout);
  static int SetBuffersGeometry(int width, int height, int format);
  static int android_printf(const char *format, ...);
  static void BringToFront();
  static std::vector<GLuint> &GetTexturePool();

  static int GetBatteryLevel();
  static bool EnableWakeLock(bool on);
  static bool HasFocus() { return m_hasFocus; }
  static bool IsResumed() { return m_isResumed; }
  static void CheckHeadsetPlugged();
  static bool IsHeadsetPlugged();

  static bool StartActivity(const std::string &package, const std::string &intent = std::string(), const std::string &dataType = std::string(), const std::string &dataURI = std::string());
  static std::vector <androidPackage> GetApplications();

  /*!
   * \brief If external storage is available, it returns the path for the external storage (for the specified type)
   * \param path will contain the path of the external storage (for the specified type)
   * \param type optional type. Possible values are "", "files", "music", "videos", "pictures", "photos, "downloads"
   * \return true if external storage is available and a valid path has been stored in the path parameter
   */
  static bool GetExternalStorage(std::string &path, const std::string &type = "");
  static bool GetStorageUsage(const std::string &path, std::string &usage);
  static int GetMaxSystemVolume();
  static float GetSystemVolume();
  static void SetSystemVolume(float percent);
  static void InitDirectories();

  static void SetRefreshRate(float rate);
  static void SetDisplayMode(int mode);
  static int GetDPI();

  static CRect MapRenderToDroid(const CRect& srcRect);
  static CPoint GetDroidToGuiRatio();

  static int WaitForActivityResult(const CJNIIntent &intent, int requestCode, CJNIIntent& result);

  // Playback callbacks
  static void OnPlayBackStarted();
  static void OnPlayBackPaused();
  static void OnPlayBackResumed();
  static void OnPlayBackStopped();
  static void OnPlayBackEnded();

  // input device methods
  static void RegisterInputDeviceCallbacks(IInputDeviceCallbacks* handler);
  static void UnregisterInputDeviceCallbacks();
  static const CJNIViewInputDevice GetInputDevice(int deviceId);
  static std::vector<int> GetInputDeviceIds();

  static void RegisterInputDeviceEventHandler(IInputDeviceEventHandler* handler);
  static void UnregisterInputDeviceEventHandler();
  static bool onInputDeviceEvent(const AInputEvent* event);

  static bool WaitVSync(unsigned int milliSeconds);
  static uint64_t GetVsyncTime() { return m_vsynctime; }

  bool getVideosurfaceInUse();
  void setVideosurfaceInUse(bool videosurfaceInUse);

protected:
  // limit who can access Volume
  friend class CAESinkAUDIOTRACK;

  static int GetMaxSystemVolume(JNIEnv *env);
  static bool AcquireAudioFocus();
  static bool ReleaseAudioFocus();

private:
  static CXBMCApp* m_xbmcappinstance;
  static bool HasLaunchIntent(const std::string &package);
  std::string GetFilenameFromIntent(const CJNIIntent &intent);
  void run();
  void stop();
  void SetupEnv();
  static void SetRefreshRateCallback(CVariant *rate);
  static void SetDisplayModeCallback(CVariant *mode);
  static ANativeActivity *m_activity;
  static CJNIWakeLock *m_wakeLock;
  static int m_batteryLevel;
  static bool m_hasFocus;
  static bool m_isResumed;
  static bool m_hasAudioFocus;
  static bool m_headsetPlugged;
  static IInputDeviceCallbacks* m_inputDeviceCallbacks;
  static IInputDeviceEventHandler* m_inputDeviceEventHandler;
  bool m_videosurfaceInUse;
  bool m_firstrun;
  bool m_exiting;
  pthread_t m_thread;
  static CCriticalSection m_applicationsMutex;
  static std::vector<androidPackage> m_applications;
  static std::vector<CActivityResultEvent*> m_activityResultEvents;

  static ANativeWindow* m_window;
  static CEvent m_windowCreated;
  static std::vector<GLuint> m_texturePool;

  static CJNIAudioDeviceInfos m_audiodevices;

  static uint64_t m_vsynctime;
  static CEvent m_vsyncEvent;

  bool XBMC_DestroyDisplay();
  bool XBMC_SetupDisplay();
};
