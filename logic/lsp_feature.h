#ifndef LSP_FEATURE_H
#define LSP_FEATURE_H

#include <string>
#include <string_view>
#include <vector>

struct LSP_feature {
    std::string name;
    //Trigger trigger;
    //Action action;
    static LSP_feature *lookup(std::string_view name);
};

#endif // LSP_FEATURE_H