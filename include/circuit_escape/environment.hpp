template<std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
public:
    using grid_type = Grid<Cell, Rows, Columns>;
    NavigationEnvironment(grid_type initialGrid, Position start, int initialEnergy, std::size_t turnLimit) {}

    void reset(std::uint32_t seed) {}
    [[nodiscard]] Observation state() const {}
    [[nodiscard]] std::vector<Action> availableActions() const {}
    [[nodiscard]] bool isFinished() const noexcept {}
    [[nodiscard]] StepResult step(Action action) {}
    [[nodiscard]] const grid_type& grid() const noexcept {}
};