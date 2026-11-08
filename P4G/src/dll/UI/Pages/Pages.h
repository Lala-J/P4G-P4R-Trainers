#pragma once
// One function per trainer page. Each draws inside the page area of the modal.

namespace Pages {

void Dashboard();
void CaseFile();
void SocialLinks();
void Protagonist();
void Inventory();
void Personas();
void BattlePage();
void Advanced();
void EnemyIntel();
void LevelsExp();
void FieldDungeon();
void Romance();
void Today();
void IntelHud();
void Settings();
void About();

// Shown at the top of pages that need a loaded save.
bool RequireSave();

}  // namespace Pages
