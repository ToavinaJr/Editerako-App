#ifndef MAINWINDOW_SIMPLE_H
#define MAINWINDOW_SIMPLE_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include "codeeditor.h"

class SimpleMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    SimpleMainWindow(QWidget *parent = nullptr);
    ~SimpleMainWindow();

private slots:
    void openFile();
    void saveFile();
    void testCppHighlighting();

private:
    CodeEditor *codeEditor;
    QString currentFileName;

    void setupUI();
    void setupMenus();
};

#endif // MAINWINDOW_SIMPLE_H