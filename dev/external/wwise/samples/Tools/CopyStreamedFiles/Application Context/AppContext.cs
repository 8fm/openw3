////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using AK.Wwise.InfoFile;
using System.IO;

namespace AkCopyStreamedFiles
{
    /// <summary>
    /// Application context in Generate mode.
    /// A progress bar is shown if in_bVerbose is true only.
    /// The user shall call Application.Run() with this context in order to create the 
    /// message queue needed to process and wait for the "Close" button of the progress dialog.
    /// If there is no progress dialog, this.RunApplicationLoop is false and should not
    /// be called, so that the process ends by itself when the generation is complete.
    /// </summary>
    class AppContext : ApplicationContext
    {

        public void LogMsgHandler(object sender, LogMsgEventArgs e)
        {
            m_Log.WriteLine(e.Msg);
        }

        /// <summary>
        /// Copy files to the specified folder using information from the specified
        /// info file.
        /// </summary>
        /// <param name="in_szInfoFile">Path to the SoundBanksInfo.xml file.</param>
        /// <param name="in_szOutputPath">Path to the target folder.</param>
        /// <returns>0 if succesful, non-zero otherwise.</returns>
        public int Process(
            string in_szInfoFile,
            string in_szOutputPath,
			IList<string> in_banks,
			IList<string> in_languages)
        {
            m_Log = new StreamWriter(in_szOutputPath+"\\AkCopyStreamFiles.log");
            StreamedFilesCopier copier = new StreamedFilesCopier();

            int iReturn = 0;
            try
            {
                // In order to successfully start copying:
                // - an info file must have been specified;
                // - the target path must have been specified.
                if (in_szInfoFile.Length > 0 && in_szOutputPath.Length > 0)
                {
                    SoundBanksInfo data = InfoFileHelpers.LoadInfoFile(in_szInfoFile);
                    if (data != null)
                    {
                        Copy(copier, data, in_szInfoFile, in_szOutputPath, in_banks, in_languages);
                    }
                }
                else
                {
                    string szInvalidParam = "";
                    if (in_szInfoFile.Length == 0)
                        szInvalidParam = "No info file specified (-info).\n";
                    if (in_szOutputPath.Length == 0)
                        szInvalidParam += "No output path specified (-outputpath).\n";
                    throw new Exception(
                        "Some of the required input parameters are not valid: "
                        + szInvalidParam
                        + ". Usage: AkCopyStreamedFiles.exe -info <path-to-soundsbanksinfo.xml> -outputpath <output-folder> [-autoclose] [-hideprogressui]");
                }
            }
            catch (Exception ex)
            {
                // Error.
                string szMsg = "Streamed Files Copying FAILED! " + ex.Message;

                m_Log.WriteLine(szMsg);

                // Dump to STDERR
                Console.Error.WriteLine(szMsg);
                iReturn = 1;
            }

            // No UI. Let user know that it does not need to start the message loop.
            m_bRunApplicationLoop = false;

            m_Log.Flush();
            m_Log.Close();
            m_Log.Dispose();

            return iReturn;
        }

        /// <summary>
        /// Copy files to the specified folder using information from the specified
        /// info file.
        /// </summary>
        /// <param name="in_copier">Instance of StreamedFilesCopier that will be used to copy files.</param>
        /// <param name="in_data">Info from SoundBanksInfo.xml.</param>
        /// <param name="in_szInfoFile">Path to SoundBanksInfo.xml.</param>
        /// <param name="in_szOutputPath">Output folder.</param>
        private void Copy(
            StreamedFilesCopier in_copier,
            SoundBanksInfo in_data,
            string in_szInfoFile,
            string in_szOutputPath,
			IList<string> in_banks,
			IList<string> in_languages)
        {
            Message("Copying Streamed Files to '" + in_szOutputPath + "'...");

			bool bResult = in_copier.Copy(in_data, in_szOutputPath, in_banks, in_languages);

            if (!bResult)
            {
                string message = "WARNING: some streamed files referenced in " + in_szInfoFile + " could not be copied.";

                m_Log.WriteLine(message);
                Console.Error.WriteLine(message);
            }
        }

        private void OnUIClosed(object sender, EventArgs e)
        {
            ExitThread();
        }

        public bool RunApplicationLoop
        {
            get { return m_bRunApplicationLoop; }
        }

        internal void Message(string in_szMsg)
        {
            Console.WriteLine(in_szMsg);
        }

        private bool m_bRunApplicationLoop = true;
        private StreamWriter m_Log;
    }
}
