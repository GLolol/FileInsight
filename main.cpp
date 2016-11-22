#include "fileinsight.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    // Main function: create a Qt application using the command line arguments given
    QApplication a(argc, argv);

    // Handle command line arguments such as --help and a filename list
    QCommandLineParser parser;
    parser.setApplicationDescription("FileInsight is a GUI frontend for file type detection.");
    parser.addHelpOption();
    parser.addPositionalArgument("files", QCoreApplication::translate("main",
                                 "Optional list of filenames to open."), "[filenames...]");
    parser.process(a);

    // Create an instance of the FileInsight window, and display it
    FileInsight w;
    QStringList args = parser.positionalArguments();
    int argcount = args.count();

    // Open every file, and spawn a new tab unless we're at the last file.
   for (int i=0; i<argcount; i++) {
        w.openFile(args[i]);

        if (argcount-1 != i) {
            w.newTab();
        }
    }

    w.show();

    return a.exec(); // Run the Qt app and exit with its return code
}
