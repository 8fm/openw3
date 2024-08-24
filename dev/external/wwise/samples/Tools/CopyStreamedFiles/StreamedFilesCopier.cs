////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AK.Wwise.InfoFile;
using System.Collections;
using System.Linq;

namespace AkCopyStreamedFiles
{
    /// <summary>
    /// ProgressNotificationsDispatcher interface. The StreamedFilesCopier 
    /// implements this interface to allow other objects to dispatch notifications 
    /// to progress listeners.
    /// </summary>
    interface IProgressNotificationsDispatcher
    {
        /// <summary>
        /// Send a notification in order to signal that a generation substep 
        /// has been completed.
        /// </summary>
        void NotifySubstep();
        
        /// <summary>
        /// Send a message about the generation progress.
        /// </summary>
        /// <param name="in_szMsg"></param>
        void NotifyLogMsg(string in_szMsg);
    }

    internal class StreamedFilesCopier
        : IProgressNotificationsDispatcher
    {
        // Streamed Files Copying events.
        /// <summary>
        /// Register to this event to be notified when the generation step changes.
        /// </summary>
        public event EventHandler<StepChangeEventArgs> StepChange;
        /// <summary>
        /// Register to this event to be notified when the generation substep is incremented.
        /// </summary>
        public event EventHandler<EventArgs> SubStep;
        /// <summary>
        /// Register to this event to be notified of messages about the generation process.
        /// </summary>
        public event EventHandler<LogMsgEventArgs> LogMsg;

		/// <summary>
        /// Unique key for items in the StreamedFiles list of the info file
		/// </summary>
        private class StreamedFileKey
        {
            /// <summary>
            /// Source ID (Id attribute of the File element)
            /// </summary>
            private uint _id;

            /// <summary>
            /// Target Language (Language attribute of the File element)
            /// </summary>
            private string _lang;

            /// <summary>
            /// Constructor
            /// </summary>
            public StreamedFileKey(uint id, string lang)
            {
                _id = id;
                _lang = lang;
            }

            /// <summary>
            /// Comparer, to use StreamedFileKey as a key in a Dictionary
            /// </summary>
            public class EqualityComparer : IEqualityComparer<StreamedFileKey>
            {
                /// <summary>
                /// Determines whether the specified objects are equal.
                /// </summary>
                public bool Equals(StreamedFileKey x, StreamedFileKey y)
                {
                    // Compare both members
                    return x._id == y._id && x._lang == y._lang;
                }

                /// <summary>
                /// Returns a hash code for the specified object.
                /// </summary>
                public int GetHashCode(StreamedFileKey x)
                {
                    // In general the ID is unique. Only when a Voice doesn't have a source
                    // in a given language and it falls back on the reference language will
                    // we have two entries with the same ID. So using the hash code from
                    // the ID should be pretty good...
                    return x._id.GetHashCode();
                }
            }
        }

        /// <summary>
        /// Copy streamed files.
        /// </summary>
        /// <param name="in_soundbanksInfo">Soundbanks data model.</param>
        /// <param name="in_szOutputPath">Full path of the base folder where
        /// streamed files will be copied (language-specific files will be
        /// in subfolders).</param>
        /// <returns>Returns true when no files are missing.</returns>
		public bool Copy(
			SoundBanksInfo in_soundbanksInfo, 
			string in_szOutputPath, 
			IList<string> in_banks,
			IList<string> in_languages)
        {
            bool bResult = true;

            OnStepChange(in_soundbanksInfo.StreamedFiles.FileCollection.Count, "Copying Streamed Files");

			// By default, copy all files
            IEnumerable<AK.Wwise.InfoFile.File> filesToCopy = in_soundbanksInfo.StreamedFiles.FileCollection.Cast<AK.Wwise.InfoFile.File>();

			// If soundbank is specified, override the list
			if (in_banks != null)
			{
				// First build a map id/lang->stream
                Dictionary<StreamedFileKey, FileDescriptorType> streamsByIdAndLanguage = new Dictionary<StreamedFileKey, FileDescriptorType>(
                    new StreamedFileKey.EqualityComparer());
                foreach (FileDescriptorType stream in in_soundbanksInfo.StreamedFiles)
                {
                    // The StreamedFiles list contains items for which the ID+Language combination is unique
                    streamsByIdAndLanguage.Add(new StreamedFileKey(stream.Id, stream.Language), stream);
                }

				// Then build the list of stream by looking at the referenced streams ion soundbanks
                List<AK.Wwise.InfoFile.File> streams = new List<AK.Wwise.InfoFile.File>();
				foreach (SoundBank bank in in_soundbanksInfo.SoundBanks)
				{
					if (in_banks.Contains(bank.ShortName.ToLower()))
					{
						foreach (ReferencedStreamedFilesFile refFile in bank.ReferencedStreamedFiles)
						{
							FileDescriptorType stream;
                            StreamedFileKey fileKey = new StreamedFileKey(refFile.Id, bank.Language);
                            if (streamsByIdAndLanguage.TryGetValue(fileKey, out stream))
                            {
                                streams.Add(stream as AK.Wwise.InfoFile.File);
                                streamsByIdAndLanguage.Remove(fileKey);
                            }
                            else if (bank.Language != "SFX")
                            {
                                // For mixed banks (SFX+language specific), do a second check for the SFX bank
                                fileKey = new StreamedFileKey(refFile.Id, "SFX");
                                if (streamsByIdAndLanguage.TryGetValue(fileKey, out stream))
                                {
                                    streams.Add(stream as AK.Wwise.InfoFile.File);
                                    streamsByIdAndLanguage.Remove(fileKey);
                                }
                            }
						} 
					}
				}

				// Override list of files
				filesToCopy = streams;
			}

            // Filter out RSX files
            filesToCopy = filesToCopy.Where(f => f.RSX == false);

            foreach (AK.Wwise.InfoFile.File stream in filesToCopy)
            {
                string szDestFilePath;
                if (stream.Language == "SFX")
                {
                    // Non-language-specific files go in the root of the output folder
                    szDestFilePath = in_szOutputPath;
                }
                else
                {
                    // Skip languages we don't want
                    if (in_languages != null && !in_languages.Contains(stream.Language.ToLower()))
                        continue;

                    // Add language-specific subfolder
                    szDestFilePath = System.IO.Path.Combine(in_szOutputPath, stream.Language);
                }

                string szSourceFilePath = System.IO.Path.Combine(in_soundbanksInfo.RootPaths.SourceFilesRoot, stream.Path);

                try
                {                    
                    if (!System.IO.Directory.Exists(szDestFilePath))
                    {
                        System.IO.Directory.CreateDirectory(szDestFilePath);
                    }

                    szDestFilePath = System.IO.Path.Combine(szDestFilePath, GetFileName(stream));

                    // Avoid copying if the source and target files are identical
					if ( !IsSameStreamedFile( szSourceFilePath, szDestFilePath ) )
                    {
                        if (System.IO.File.Exists(szDestFilePath))
                        {
                            System.IO.File.Delete(szDestFilePath);
                        }

						System.IO.File.Copy( szSourceFilePath, szDestFilePath );

						NotifyLogMsg( " - Copied '" + szSourceFilePath + "' to '" + szDestFilePath + "'." );
                    }
                }
                catch (Exception ex)
                {
					string message = "ERROR: Unable to copy '" + szSourceFilePath + "' to '" + szDestFilePath + "'. Reason: " + ex.Message;

                    Console.Error.WriteLine(message);
                    NotifyLogMsg(" - " + message);
                    
                    bResult = false;
                }

                OnSubStep();
            }

            return bResult;
        }


        /// <summary>
        /// Get the target filename for the specified file.
        /// </summary>
        /// <param name="in_stream">File Descriptor.</param>
        /// <returns>Filename built from the specified file descriptor.</returns>
        private string GetFileName(FileDescriptorType in_stream)
        {
            return in_stream.Id.ToString() + System.IO.Path.GetExtension(in_stream.Path);
        }

        /// <summary>
        /// Check if the 2 files are identical or not. This function checks the
        /// filelength and last modified time to determine if the files are
        /// the same.
        /// </summary>
        /// <param name="in_szFile1">Full path to first file.</param>
        /// <param name="in_szFile2">Full path to second file.</param>
        /// <returns>true if both files exist and are considered to be identical. false otherwise.</returns>
        private bool IsSameStreamedFile(string in_szFile1, string in_szFile2 )
        {
            if (!System.IO.File.Exists(in_szFile1) || !System.IO.File.Exists(in_szFile2))
            {
                return false;
            }

            FileInfo fileInfo1 = new FileInfo(in_szFile1);
            FileInfo fileInfo2 = new FileInfo(in_szFile2);

            return ( fileInfo1.LastWriteTimeUtc == fileInfo2.LastWriteTimeUtc )
                && ( fileInfo1.Length == fileInfo2.Length );
        }

        // ProgressNotificationsDispatcher implementation.
        /// <summary>
        /// ProgressNotificationsDispatcher implementation.
        /// Dispatches the SubStep event.
        /// </summary>
        public void NotifySubstep()
        {
            OnSubStep();
        }

        /// <summary>
        /// ProgressNotificationsDispatcher implementation.
        /// Dispatches the LogMsg event.
        /// </summary>
        /// <param name="in_szMsg">Message</param>
        public void NotifyLogMsg(string in_szMsg)
        {
            OnLogMsg(in_szMsg);
        }

        /// <summary>
        /// Progress.StopRequestedEventHandler.
        /// Handles "abort generation" events.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void StopRequestedHandler(object sender, EventArgs e)
        {
            m_bStopRequested = true;
        }

        /// <summary>
        /// Returns true if someone requested that the generation be stopped.
        /// </summary>
        public bool StopRequested
        {
            get { return m_bStopRequested; }
        }
        
        /// <summary>
        /// Dispatch the StepChange event.
        /// </summary>
        /// <param name="in_iNumSubSteps">Number of substeps contained in this generation step</param>
        /// <param name="in_szStepName">Name of the generation step</param>
        internal void OnStepChange(int in_iNumSubSteps, string in_szStepName)
        {
            if (StepChange != null)
                StepChange(this, new StepChangeEventArgs(in_iNumSubSteps, in_szStepName));
        }

        /// <summary>
        /// Dispatch the SubStep event.
        /// Note: External stop requests are processed here. An exception is
        /// thrown in order to abort the generation.
        /// </summary>
        internal void OnSubStep()
        {
            if (SubStep != null)
                SubStep(this, new EventArgs());

            // Check for stop request now.
            if (StopRequested)
            {
                throw new Exception("Streamed Files Copying stopped by user request.");
            }
        }

        /// <summary>
        /// Dispatch the LogMsg event.
        /// </summary>
        /// <param name="in_szMsg">Message</param>
        internal void OnLogMsg(string in_szMsg)
        {
            if (LogMsg != null)
                LogMsg(this, new LogMsgEventArgs(in_szMsg));
        }

        private bool m_bStopRequested = false;
    }

    /// <summary>
    /// Custom event definition for StepChange event.
    /// Provides the number of substeps and name of the step.
    /// </summary>
    public class StepChangeEventArgs : EventArgs
    {
        public StepChangeEventArgs(int in_iNumSubSteps, string in_szStepName)
            : base()
        {
            m_iNumSubSteps = in_iNumSubSteps;
            m_szStepName = in_szStepName;
        }
        public int NumSubSteps
        {
            get { return m_iNumSubSteps; }
            set { m_iNumSubSteps = value; }
        }
        public string StepName
        {
            get { return m_szStepName; }
            set { m_szStepName = value; }
        }
        private int m_iNumSubSteps;
        private string m_szStepName;
    }

    /// <summary>
    /// Custom event definition for LogMsg event.
    /// Provides the message.
    /// </summary>
    public class LogMsgEventArgs : EventArgs
    {
        public LogMsgEventArgs(string in_szMsg)
            : base()
        {
            m_szMsg = in_szMsg;
        }
        public string Msg
        {
            get { return m_szMsg; }
            set { m_szMsg = value; }
        }
        private string m_szMsg;
    }
}
