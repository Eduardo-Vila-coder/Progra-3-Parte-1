#include "circuit_escape/position.hpp"
#include "circuit_escape/environment.hpp"
//#include <ftxui/component/event.hpp>
//#include <ftxui/dom/elements.hpp>
struct QuitCommand
{
};
struct HelpCommand
{
};
using UiCommand = std::variant<Action, QuitCommand, HelpCommand>;
enum class RenderMode
{
    emoji,
    ascii
};
class ConsoleUI
{
public:
    explicit ConsoleUI(RenderMode mode = RenderMode::emoji);
    std::optional<UiCommand> translate(const ftxui::Event &event) const;
    ftxui::Element render(
        const NavigationEnvironment<20, 30> &environment,
        std::span<const NavigationEvent> recentEvents) const;
    ftxui::Element help() const;
};