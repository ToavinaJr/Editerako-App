#include "ui/UiHelpers.h"

#include <QBoxLayout>
#include <QInputDialog>
#include <QLayout>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QWidget>

void replacePlaceholder(QWidget *placeholder, QWidget *replacement, QWidget *fallbackParent)
{
    auto addToFallback = [replacement](QWidget *parent) {
        if (!parent) {
            return;
        }
        if (parent->layout()) {
            parent->layout()->addWidget(replacement);
        } else {
            auto *newLayout = new QVBoxLayout(parent);
            newLayout->setContentsMargins(6, 6, 6, 6);
            newLayout->addWidget(replacement);
        }
    };

    if (placeholder) {
        QLayout *parentLayout = placeholder->parentWidget() ? placeholder->parentWidget()->layout() : nullptr;
        if (parentLayout) {
            for (int i = 0; i < parentLayout->count(); ++i) {
                QLayoutItem *item = parentLayout->itemAt(i);
                if (item && item->widget() == placeholder) {
                    QLayoutItem *removed = parentLayout->takeAt(i);
                    if (removed) {
                        if (removed->widget()) {
                            removed->widget()->deleteLater();
                        }
                        delete removed;
                    }
                    if (auto *box = qobject_cast<QBoxLayout *>(parentLayout)) {
                        box->insertWidget(i, replacement);
                    } else {
                        parentLayout->addWidget(replacement);
                    }
                    return;
                }
            }
        }
        addToFallback(fallbackParent);
        return;
    }

    addToFallback(fallbackParent);
}

QString promptText(QWidget *parent,
                   const QString &title,
                   const QString &label,
                   const QString &defaultValue,
                   int minWidth,
                   int minHeight)
{
    QInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setTextValue(defaultValue);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setMinimumWidth(minWidth);
    dialog.setMinimumHeight(minHeight);
    if (!dialog.exec()) {
        return {};
    }
    return dialog.textValue();
}
