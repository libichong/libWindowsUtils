//This smart object will guarentee to call CloseHandle to cleanup the multex handle
class SmartMutex
{
    HANDLE m_hMutex;
    //static const int SmartMutex_Timeout=600*1000;

   public:

    SmartMutex::SmartMutex()
    {

        m_hMutex = CreateMutex(NULL, FALSE, L"SmartMutex");
    }
    SmartMutex::~SmartMutex()
    {
        if (m_hMutex != NULL)
            CloseHandle(m_hMutex);
    }

    BOOL SmartMutex::TryLockit()
    {
        // In rare case where mutex not created, we allow file operations
        // with no synchronization
        if (m_hMutex == NULL)
            return TRUE;


        BOOL fResult = TRUE;
        if (WaitForSingleObject(m_hMutex, 0) != WAIT_OBJECT_0) //used to be: SmartMutex_Timeout
        {
            fResult = FALSE;
        }

        return fResult;
    }

    void SmartMutex::Freeit()
    {
        if (m_hMutex != NULL) // Note: AcquireMutex succeeds even if m_hMutex is NULL
        {
            ::ReleaseMutex(m_hMutex);
        }
    }
};

BOOL VortexToggleTask::ToggleVortexKey()
{
  static SmartMutex mutex;

  if(!mutex.TryLockit())
  {
    return TRUE;
  }


  // your code

  mutex.Freeit();
  return TRUE;
}
