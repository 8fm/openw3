////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace AkCopyStreamedFiles
{
    static class Program
    {
        public const string APP_NAME = "Copy Streamed Files"; 
       

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static int Main(string[] in_args)
        {
            try
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);

                string szInfoFile = "";
                string szOutputPath = "";
                List<string> banks = null;
                List<string> languages = null;

                for (int i = 0; i < in_args.Length; i++)
                {
                    if (in_args[i][0] == '-' || in_args[i][0] == '/')
                    {
                        string arg = in_args[i].Remove(0, 1);

                        if (0 == String.Compare(arg, "info", true))
                        {
                            if (i < in_args.Length - 1)
                            {
                                szInfoFile = in_args[++i];
                            }
                            else
                            {
                                Console.Error.WriteLine("Error: Missing path to SoundBanksInfo.xml file after -info argument.");
                            }
                        }
                        else if (0 == String.Compare(arg, "outputpath", true))
                        {
                            if (i < in_args.Length - 1)
                            {
                                szOutputPath = in_args[++i];
                            }
                            else
                            {
                                Console.Error.WriteLine("Error: Missing output path after -outputpath argument.");
                            }
                        }
                        else if (0 == String.Compare(arg, "banks", true))
                        {
                            if (i < in_args.Length - 1)
                            {
                                banks = new List<string>(in_args[i + 1].ToLower().Split(" ".ToCharArray()));
                            }
                            else
                                throw new InvalidOperationException("Value expected: list of SoundBank names (space separated)");
                        }
                        else if (0 == String.Compare(arg, "languages", true))
                        {
                            if (i < in_args.Length - 1)
                                languages = new List<string>(in_args[i + 1].ToLower().Split(" ".ToCharArray()));
                            else
                                throw new InvalidOperationException("Value expected: list of language names (space separated)");
                        }
                    }
                }

                AppContext appCtx = new AppContext();
                int iReturn = appCtx.Process(szInfoFile, szOutputPath, banks, languages);

                if (appCtx.RunApplicationLoop)
                    Application.Run(appCtx);

                return iReturn;
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("Unexpected error: " + e.Message);
                return 1;
            }
        }
    }
}