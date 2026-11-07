#pragma once
// Every change the trainer makes is recorded with a way to undo it.

#include <functional>
#include <string>
#include <vector>

namespace Journal {

struct Entry {
    int id;
    std::string what;            // "Money ¥24011 → ¥99999"
    std::string when;            // "4/21 After School, 23:41"
    std::function<void()> undo;  // runs on the game thread
    bool undone = false;
};

void Record(std::string what, std::function<void()> undo);
const std::vector<Entry>& Entries();
void Undo(int id);               // queued on the game thread
void UndoAll();

}  // namespace Journal
