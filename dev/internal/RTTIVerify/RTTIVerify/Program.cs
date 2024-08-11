using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace RTTIVerify
{
	class Program
	{
		static string directory;

		static ArrayList includeFiles;
		static ArrayList cppFiles;

		static int enumCount = 0;
		static int notImplementedCount = 0;

		static void Main(string[] args)
		{
			string projectPath = "";
			if (args.Length > 0)
			{
				projectPath = args[0];
			}

			FileInfo projectFile = new FileInfo(projectPath);
			if (!projectFile.Exists)
			{
				Environment.Exit(666);
			}

			directory = projectFile.Directory.FullName + "\\";

			includeFiles = new ArrayList();
			cppFiles = new ArrayList();

			XmlDocument doc = new XmlDocument();
			doc.Load(projectPath);

			XmlNamespaceManager nsmgr = new XmlNamespaceManager( doc.NameTable );
			nsmgr.AddNamespace( "ns", "http://schemas.microsoft.com/developer/msbuild/2003");

			XmlNodeList nodes = doc.SelectNodes("//ns:ClInclude", nsmgr);
			foreach (XmlNode node in nodes)
			{
				XmlAttributeCollection attrCol = node.Attributes;
				XmlAttribute attribute = attrCol["Include"];

				if (attribute != null)
				{
					includeFiles.Add(attribute.Value);
				}
			}

			nodes = doc.SelectNodes("//ns:ClCompile", nsmgr);
			foreach (XmlNode node in nodes)
			{
				XmlAttributeCollection attrCol = node.Attributes;
				XmlAttribute attribute = attrCol["Include"];

				if (attribute != null)
				{
					cppFiles.Add(attribute.Value);
				}
			}

			foreach (string file in includeFiles)
			{
				ParseFile(new FileInfo(directory + file));
			}

			Console.WriteLine("{0:G} enums found.", enumCount);
			Console.WriteLine("{0:G} enums not implemented.", notImplementedCount);

			Environment.Exit(notImplementedCount);
		}

		static void ParseFile(FileInfo file)
		{
			if( file.Exists )
			{
				StreamReader reader = file.OpenText();

				string line;
				int lineNumber = 0;
				char[] trimChars = { '(', ')', ';', ' ', '\t' };
				while ((line = reader.ReadLine()) != null)
				{
					++lineNumber;
					line = line.Trim();

					if (line.StartsWith("BEGIN_ENUM_RTTI"))
					{
						line = line.Replace("BEGIN_ENUM_RTTI", "");
						line = line.Trim(trimChars);

						++enumCount;

						SearchForImplementation(file, line, lineNumber);
					}
				}

				reader.Close();
			}
		}

		static void SearchForImplementation(FileInfo header, string enumName, int lineNumber)
		{
			string fileName = header.Name.Replace(".h", ".cpp");
			FileInfo cpp = new FileInfo(directory + fileName);

			if (cpp.Exists)
			{
				if (FindImplementation(cpp, enumName))
				{
					return;
				}
			}

			foreach (string file in cppFiles)
			{
				if (FindImplementation(new FileInfo(directory + file), enumName))
				{
					return;
				}
			}

			++notImplementedCount;
			Console.WriteLine(header.FullName + "(" + lineNumber.ToString() + "): error C666: RTTI enum implementation missing - " + enumName);
		}

		static bool FindImplementation( FileInfo file, string enumName )
		{
			if( file.Exists )
			{
				StreamReader reader = file.OpenText();

				string line;
				while ((line = reader.ReadLine()) != null)
				{
					line = line.Trim();

					if (line.StartsWith("IMPLEMENT_RTTI_ENUM") && line.Contains(enumName))
					{
						reader.Close();
						return true;
					}
				}

				reader.Close();
			}

			return false;
		}
	}
}