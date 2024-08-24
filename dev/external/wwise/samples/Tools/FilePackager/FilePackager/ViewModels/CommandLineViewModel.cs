﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FilePackager.Base;
using FilePackager.Models;
using FilePackager.Packaging;

namespace FilePackager.ViewModels
{
    public class CommandLineViewModel : ViewModelBase
    {
        public bool IsGenerateMode { get; set; }

        public string ProjectPath { get; set; }

        // Generate specific options
        public string InfoFilePath { get; set; }
        public string SinglePackagePath { get; set; }
        public string BankPackagePath { get; set; }
        public string StreamPackagePath { get; set; }
        public string RSXPackagePath { get; set; }
        public uint BlockSize { get; set; }
        public uint BlockSizeRSX { get; set; }
        public IEnumerable<string> Banks { get; set; }
        public IEnumerable<string> Languages { get; set; }

        public string SourceFilesPath { get; set; }
        public string SoundBanksPath { get; set; }

        public bool ShowProgressUi { get; set; }

        private static CommandLineViewModel _instance = null;

        public CommandLineViewModel(string[] arguments)
        {
            if (_instance != null)
                throw new InvalidOperationException("Command line instantiated multiple times");
            _instance = this;

            BlockSize = 1;
            BlockSizeRSX = 128;
            IsGenerateMode = false;
            ShowProgressUi = true;
            Parse(arguments);
        }

        public static CommandLineViewModel Instance
        {
            get { return _instance; }
        }

        private void Parse(string[] arguments)
        {
            int i = 0;
            try
            {
                for (; i < arguments.Length; i++)
                {
                    if (0 == String.Compare(arguments[i], "-generate", true))
                    {
                        IsGenerateMode = true;
                    }
                    else if (arguments[i].EndsWith(".wfpproj"))
                    {
                        ProjectPath = arguments[i];
                    }
                    else if (0 == String.Compare(arguments[i], "-info", true))
                    {
                        if (i < arguments.Length - 1)
                            InfoFilePath = arguments[++i];
                        else
                            throw new InvalidOperationException("-info: Value expected <SoundbanksInfo.xml>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-output", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0)
                            SinglePackagePath = arguments[++i].Trim();
                        else
                            throw new InvalidOperationException("-output: Value expected <PackageName.pck>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-output_stm", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0)
                            StreamPackagePath = arguments[++i].Trim();
                        else
                            throw new InvalidOperationException("-output_stm: Value expected <PackageName.pck>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-output_bnk", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0)
                            BankPackagePath = arguments[++i].Trim();
                        else
                            throw new InvalidOperationException("-output_bnk: Value expected <PackageName.pck>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-output_rsx", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0 && !arguments[i + 1].Trim().StartsWith("-"))
                            RSXPackagePath = arguments[++i].Trim();
                        else
                            RSXPackagePath = ".\\";// Default value is default directory
                    }
                    else if (0 == String.Compare(arguments[i], "-blocksize", true))
                    {
                        if (i < arguments.Length - 1)
                            BlockSize = uint.Parse(arguments[++i]);
                        else
                            throw new InvalidOperationException("-blocksize: Value expected <integer value>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-blocksize_rsx", true))
                    {
                        if (i < arguments.Length - 1)
                            BlockSizeRSX = uint.Parse(arguments[++i]);
                        else
                            throw new InvalidOperationException("-blocksize_rsx: Value expected <integer value>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-hideprogressui", true))
                    {
                        if (i < arguments.Length - 1)
                            ShowProgressUi = !bool.Parse(arguments[++i]);
                        else
                            throw new InvalidOperationException("-hideprogressui: Value expected <true or false>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-soundbanks_dir", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0)
                            SoundBanksPath = arguments[++i].Trim();
                        else
                            throw new InvalidOperationException("-soundbanks_dir: Value expected <directory>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-cache_dir", true))
                    {
                        if (i < arguments.Length - 1 && arguments[i + 1].Trim().Length > 0)
                            SourceFilesPath = arguments[++i].Trim();
                        else
                            throw new InvalidOperationException("-cache_dir: Value expected <directory>.");
                    }
                    else if (0 == String.Compare(arguments[i], "-banks", true))
                    {
                        if (i < arguments.Length - 1)
                            Banks = arguments[++i].Split(" ".ToCharArray());
                        else
                            throw new InvalidOperationException("-banks: Value expected <list of SoundBank names> (space separated)");
                    }
                    else if (0 == String.Compare(arguments[i], "-languages", true))
                    {
                        if (i < arguments.Length - 1)
                            Languages = arguments[++i].Split(" ".ToCharArray());
                        else
                            throw new InvalidOperationException("-languages: Value expected  <list of language names> (space separated)");
                    }
                }
            }
            catch (Exception exception)
            {
                string message = arguments[i] + " : " + exception.Message;
                MainViewModel.Instance.Application.ShowMessage(message, Severity.Error);

                throw new ArgumentException("Could not parse arguments");
            }
        }

        public void SetupProject(ProjectViewModel projectViewModel)
        {
            if (string.IsNullOrEmpty(InfoFilePath))
                throw new InvalidOperationException("Info file path not specified, use the -info argument");

            Project project = projectViewModel.Project;
            project.InfoFilePath = InfoFilePath;
           
            if (string.IsNullOrEmpty(SinglePackagePath) &&
                string.IsNullOrEmpty(BankPackagePath) &&
                string.IsNullOrEmpty(StreamPackagePath) &&
                string.IsNullOrEmpty(RSXPackagePath) )
                throw new InvalidOperationException("At least one of the output file must be specified (-output, -output_stm, -output_bnk, -output_rsx)");

            // Filter to keep only the item listed on command line
            IEnumerable<ContentItemViewModel> sourceItems = projectViewModel.ContentItems;

            if (Banks != null)
            {
                sourceItems = sourceItems.Where(ci => Banks.Contains(ci.FileName, new StringEqualityComparer(StringComparison.OrdinalIgnoreCase)));
            }

            if (Languages != null)
            {
                sourceItems = sourceItems.Where(ci => Languages.Contains(ci.Language, new StringEqualityComparer(StringComparison.OrdinalIgnoreCase)));
            }

            // Single package
            if (!string.IsNullOrEmpty(SinglePackagePath))
            {
                Package package = new Package();
                package.Name = SinglePackagePath;
                package.Items.AddRange(sourceItems
                    .Select(i => new PackageContentItem(i.Id, i.Language, i.FileName)));

                project.ManualPackagingInfo.Packages.Add(package);
            }

            // Bank package
            if (!string.IsNullOrEmpty(BankPackagePath))
            {
                Package package = new Package();
                package.Name = BankPackagePath;
                package.Items.AddRange(sourceItems
                    .Where(i => i.FileType == FileType.SoundBank)
                    .Select(i => new PackageContentItem(i.Id, i.Language, i.FileName) { InclusionMode = InclusionMode.Bank }));

                project.ManualPackagingInfo.Packages.Add(package);
            }
            
            // Stream package
            if (!string.IsNullOrEmpty(StreamPackagePath))
            {
                Package package = new Package();
                package.Name = StreamPackagePath;
                package.Items.AddRange(sourceItems
                    .Where(i => i.FileType == FileType.StreamedFile)
                    .Select(i => new PackageContentItem(i.Id, i.Language, i.FileName)));

                project.ManualPackagingInfo.Packages.Add(package);
            }

            // RSX 
            if (!string.IsNullOrEmpty(RSXPackagePath))
            {
                projectViewModel.ManualPackagingInfo.CreateRSXPackages();
            }

            projectViewModel.ManualPackagingInfo.BlockSize = BlockSize;
            projectViewModel.ManualPackagingInfo.BlockSizeRSX = BlockSizeRSX;
        }
    }
}
