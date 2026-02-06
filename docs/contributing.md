# Contributing Guidelines

Thank you for considering contributing to the Warehouse Management Solution! This document provides guidelines and best practices for contributing.

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers
- Focus on constructive feedback
- Collaborate openly

## Getting Started

1. **Fork the repository**
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/warehouse-management.git
   cd warehouse-management
   ```
3. **Set up development environment**:
   - Follow setup instructions in component READMEs
   - Install required tools and dependencies
4. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Workflow

### 1. Make Your Changes

- Write clean, maintainable code
- Follow coding standards for each technology
- Add tests for new functionality
- Update documentation as needed

### 2. Test Your Changes

```bash
# C++ services
cd services/cpp/{service-name}
mkdir build && cd build
cmake .. && make test

# C# services
cd services/csharp/{service-name}
dotnet test

# Vue applications
cd apps/{app-name}
npm run test
```

### 3. Commit Your Changes

Follow conventional commit format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples**:
```bash
git commit -m "feat(inventory): add bulk stock adjustment endpoint"
git commit -m "fix(order): resolve race condition in order processing"
git commit -m "docs(readme): update installation instructions"
```

### 4. Push to Your Fork

```bash
git push origin feature/your-feature-name
```

### 5. Create a Pull Request

- Go to the original repository
- Click "New Pull Request"
- Select your fork and branch
- Fill in the PR template
- Link related issues

## Coding Standards

### C++ Code Standards

- Follow C++ Core Guidelines
- Use C++ 20 features appropriately
- Use smart pointers, avoid raw pointers
- Follow RAII principles
- Use `const` correctly
- Prefer `auto` for obvious types
- Use meaningful variable names

**Formatting**:
```bash
clang-format -i *.cpp *.h
```

**Static Analysis**:
```bash
clang-tidy *.cpp -- -std=c++20
```

### C# Code Standards

- Follow Microsoft C# Coding Conventions
- Use async/await for I/O operations
- Implement IDisposable correctly
- Use nullable reference types
- Follow SOLID principles
- Use dependency injection
- Prefer composition over inheritance

**Formatting**:
```bash
dotnet format
```

### TypeScript/Vue Code Standards

- Follow Vue 3 Style Guide
- Use TypeScript for type safety
- Use Composition API
- Write functional components
- Use props validation
- Emit events properly
- Follow naming conventions

**Linting**:
```bash
npm run lint
npm run lint:fix
```

## Testing Guidelines

### Unit Tests

- Test individual functions/methods
- Mock external dependencies
- Aim for high code coverage (>80%)
- Use descriptive test names
- Follow AAA pattern (Arrange, Act, Assert)

### Integration Tests

- Test component interactions
- Use test databases/services
- Clean up after tests
- Test happy paths and error cases

### E2E Tests

- Test critical user flows
- Use realistic scenarios
- Keep tests maintainable
- Run in CI/CD pipeline

## Documentation

### Code Documentation

**C++**:
```cpp
/**
 * @brief Calculates the total inventory value
 * @param inventory_id The ID of the inventory to calculate
 * @return The total value in the specified currency
 * @throws NotFoundException if inventory_id doesn't exist
 */
double calculate_inventory_value(int inventory_id);
```

**C#**:
```csharp
/// <summary>
/// Generates a report for the specified date range
/// </summary>
/// <param name="startDate">The start date of the report</param>
/// <param name="endDate">The end date of the report</param>
/// <returns>A Report object containing the data</returns>
/// <exception cref="ArgumentException">Thrown when date range is invalid</exception>
public Report GenerateReport(DateTime startDate, DateTime endDate)
```

**TypeScript**:
```typescript
/**
 * Fetches orders for the specified date range
 * @param startDate - The start date
 * @param endDate - The end date
 * @returns Promise resolving to array of orders
 * @throws {ApiError} When the API request fails
 */
async function fetchOrders(startDate: Date, endDate: Date): Promise<Order[]>
```

### README Updates

- Update relevant README files
- Add usage examples
- Document configuration options
- Include troubleshooting tips

### API Documentation

- Document all endpoints
- Include request/response examples
- Specify error codes
- Note authentication requirements

## Pull Request Process

### PR Checklist

- [ ] Code follows style guidelines
- [ ] All tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated
- [ ] Commit messages follow convention
- [ ] No merge conflicts
- [ ] PR description is clear
- [ ] Related issues linked

### PR Review Process

1. **Automated Checks**: CI/CD runs tests and checks
2. **Code Review**: Maintainers review code
3. **Feedback**: Address comments and suggestions
4. **Approval**: At least one maintainer approval required
5. **Merge**: Maintainer merges the PR

### Review Expectations

**As a PR Author**:
- Respond to feedback promptly
- Be open to suggestions
- Explain design decisions
- Keep PRs focused and small

**As a Reviewer**:
- Be constructive and respectful
- Provide specific feedback
- Suggest improvements
- Approve when satisfied

## Issue Reporting

### Bug Reports

Include:
- Clear description of the bug
- Steps to reproduce
- Expected vs. actual behavior
- Environment details (OS, versions)
- Screenshots if applicable
- Error logs

### Feature Requests

Include:
- Clear description of the feature
- Use cases and benefits
- Proposed implementation (if any)
- Alternatives considered

## Development Tips

### Setting Up Development Environment

1. **Use Docker for dependencies**:
   ```bash
   docker-compose up -d postgres redis rabbitmq
   ```

2. **Use IDE extensions**:
   - C++: clangd, CMake Tools
   - C#: C# Dev Kit
   - Vue: Volar, ESLint

3. **Set up pre-commit hooks**:
   ```bash
   # Install pre-commit
   pip install pre-commit
   pre-commit install
   ```

### Debugging

**C++ Services**:
```bash
# Use GDB
gdb ./build/service-name
# Or use IDE debugger (VS Code, CLion)
```

**C# Services**:
```bash
# Use VS Code debugger or Visual Studio
dotnet run --configuration Debug
```

**Vue Applications**:
```javascript
// Use Vue DevTools browser extension
// Use console.log or debugger statements
// Use browser DevTools
```

## Release Process

1. Update version numbers
2. Update CHANGELOG.md
3. Create release branch
4. Tag release
5. Deploy to staging
6. Run smoke tests
7. Deploy to production
8. Monitor metrics

## Community

- **Discussions**: Use GitHub Discussions for questions
- **Issues**: Use GitHub Issues for bugs and features
- **Chat**: Join our Discord/Slack (if available)
- **Email**: Contact maintainers at [email]

## Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Mentioned in release notes
- Credited in documentation

Thank you for contributing! 🎉
