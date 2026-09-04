#ifndef EDITERAKO_FILEEXPLORERROLES_H
#define EDITERAKO_FILEEXPLORERROLES_H

#include <qnamespace.h>

namespace FileExplorerRoles {
constexpr int kPathRole = Qt::UserRole;
constexpr int kDirRole = Qt::UserRole + 1;
constexpr int kLoadedRole = Qt::UserRole + 2;
constexpr int kNameRole = Qt::UserRole + 3;
}

#endif
