#include <trview.app/Elements/TypeInfoLookup.h>

using namespace trview;
using namespace trlevel;

// Test that looking up an ID gives the correct result.
TEST(TypeInfoLookup, LookupTR1)
{
    std::string json = "{\"games\":{\"tr1\":[{\"id\":123,\"name\":\"Test Name\"}]}}";

    TypeInfoLookup lookup(json);

    ASSERT_EQ("Test Name", lookup.lookup(LevelVersion::Tomb1, 123, 0).name);
}

// Tests that if there are identical entries for different games, the correct result is returned.
TEST(TypeInfoLookup, LookupMultipleGames)
{
    std::string json = "{\"games\":{\"tr1\":[{\"id\":123,\"name\":\"Test Name TR1\"}],\"tr2\":[{\"id\":123,\"name\":\"Test Name TR2\"}]}}";

    TypeInfoLookup lookup(json);

    ASSERT_EQ("Test Name TR1", lookup.lookup(LevelVersion::Tomb1, 123, 0).name);
    ASSERT_EQ("Test Name TR2", lookup.lookup(LevelVersion::Tomb2, 123, 0).name);
}

// Tests that if the name is missing, it still returns the number.
TEST(TypeInfoLookup, LookupMissingItem)
{
    std::string json = "{}";

    TypeInfoLookup lookup(json);

    ASSERT_EQ("123", lookup.lookup(LevelVersion::Tomb3, 123, 0).name);
}

TEST(TypeInfoLookup, LookupNormalMutantEggs)
{
    std::string json = "{\"games\":{\"tr1\":[{\"id\":163,\"name\":\"Test Name 1\"}]}}";
    TypeInfoLookup lookup(json);

    auto winged = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 0).name;
    auto shooter = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 1 << 9).name;
    auto centaur = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 2 << 9).name;
    auto torso = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 4 << 9).name;
    auto grounded = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 8 << 9).name;
    auto def = lookup.lookup(trlevel::LevelVersion::Tomb1, 163, 3 << 9).name;

    ASSERT_EQ(winged, "Mutant Egg (Winged)");
    ASSERT_EQ(shooter, "Mutant Egg (Shooter)");
    ASSERT_EQ(centaur, "Mutant Egg (Centaur)");
    ASSERT_EQ(torso, "Mutant Egg (Torso)");
    ASSERT_EQ(grounded, "Mutant Egg (Grounded)");
    ASSERT_EQ(def, "Mutant Egg (Winged)");
}

TEST(TypeInfoLookup, LookupBigMutantEggs)
{
    std::string json = "{\"games\":{\"tr1\":[{\"id\":181,\"name\":\"Test Name 2\"}]}}";
    TypeInfoLookup lookup(json);

    auto winged = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 0).name;
    auto shooter = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 1 << 9).name;
    auto centaur = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 2 << 9).name;
    auto torso = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 4 << 9).name;
    auto grounded = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 8 << 9).name;
    auto def = lookup.lookup(trlevel::LevelVersion::Tomb1, 181, 3 << 9).name;

    ASSERT_EQ(winged, "Mutant Egg (Winged)");
    ASSERT_EQ(shooter, "Mutant Egg (Shooter)");
    ASSERT_EQ(centaur, "Mutant Egg (Centaur)");
    ASSERT_EQ(torso, "Mutant Egg (Torso)");
    ASSERT_EQ(grounded, "Mutant Egg (Grounded)");
    ASSERT_EQ(def, "Mutant Egg (Winged)");
}

TEST(TypeInfoLookup, LookupNormalMutantEggsTR2)
{
    std::string json = "{\"games\":{\"tr1\":[{\"id\":163,\"name\":\"Test Name 1\"}],\"tr2\":[{\"id\":163,\"name\":\"Test Name 2\"}]}}";
    TypeInfoLookup lookup(json);

    auto winged = lookup.lookup(trlevel::LevelVersion::Tomb2, 163, 0).name;

    ASSERT_EQ(winged, "Test Name 2");
}
