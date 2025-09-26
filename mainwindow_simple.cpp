#include "mainwindow_simple.h"
#include "syntaxhighlighter.h"
#include <QApplication>
#include <QTextStream>
#include <QFileInfo>
#include <QFile>

SimpleMainWindow::SimpleMainWindow(QWidget *parent)
    : QMainWindow(parent), codeEditor(nullptr)
{
    setupUI();
    setupMenus();
    
    // Test the syntax highlighting with some sample C++ code
    testCppHighlighting();
}

SimpleMainWindow::~SimpleMainWindow()
{
}

void SimpleMainWindow::setupUI() {
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // Create code editor
    codeEditor = new CodeEditor(this);
    layout->addWidget(codeEditor);
    
    // Set up syntax highlighter for C++
    new SyntaxHighlighter(codeEditor, SyntaxHighlighter::CPP);
    
    setWindowTitle("Editerako-App - Syntax Highlighting Test");
    resize(800, 600);
}

void SimpleMainWindow::setupMenus() {
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("File");
    
    QAction *openAction = new QAction("Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &SimpleMainWindow::openFile);
    fileMenu->addAction(openAction);
    
    QAction *saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &SimpleMainWindow::saveFile);
    fileMenu->addAction(saveAction);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Test menu
    QMenu *testMenu = menuBar()->addMenu("Test");
    
    QAction *testAction = new QAction("Load C++ Sample", this);
    connect(testAction, &QAction::triggered, this, &SimpleMainWindow::testCppHighlighting);
    testMenu->addAction(testAction);
}

void SimpleMainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"), "", tr("C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            codeEditor->setPlainText(in.readAll());
            currentFileName = fileName;
            setWindowTitle(QString("Editerako-App - %1").arg(QFileInfo(fileName).fileName()));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not open file: %1").arg(fileName));
        }
    }
}

void SimpleMainWindow::saveFile() {
    if (currentFileName.isEmpty()) {
        currentFileName = QFileDialog::getSaveFileName(this,
            tr("Save File"), "", tr("C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)"));
    }
    
    if (!currentFileName.isEmpty()) {
        QFile file(currentFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << codeEditor->toPlainText();
            setWindowTitle(QString("Editerako-App - %1").arg(QFileInfo(currentFileName).fileName()));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not save file: %1").arg(currentFileName));
        }
    }
}

void SimpleMainWindow::testCppHighlighting() {
    // Sample C++ code to demonstrate syntax highlighting
    QString sampleCode = R"(#include <iostream>
#include <vector>
#include <string>
#ifndef EXAMPLE_H
#define EXAMPLE_H

namespace std {
    class Example {
    private:
        int value;
        std::string name;
        std::vector<double> data;
        
    public:
        Example(int v = 0) : value(v), name("default") {
            // Constructor with default parameter
            data.resize(10, 0.0);
        }
        
        virtual ~Example() {
            // Virtual destructor
        }
        
        void setValue(int v) {
            if (v >= 0 && v <= 100) {
                value = v;
            } else {
                throw std::runtime_error("Invalid value");
            }
        }
        
        int getValue() const { return value; }
        
        static void printInfo() {
            std::cout << "This is a sample class" << std::endl;
        }
        
        template<typename T>
        void process(T data) {
            for (auto& item : data) {
                std::cout << item << " ";
            }
            std::cout << std::endl;
        }
    };
}

int main() {
    try {
        Example obj(42);
        obj.setValue(75);
        
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        obj.process(numbers);
        
        if (obj.getValue() > 50) {
            std::cout << "Value is greater than 50" << std::endl;
        } else {
            std::cout << "Value is 50 or less" << std::endl;
        }
        
        Example::printInfo();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

#endif // EXAMPLE_H
)";
    
    codeEditor->setPlainText(sampleCode);
    setWindowTitle("Editerako-App - Sample C++ Code");
}