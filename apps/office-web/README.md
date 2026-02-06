# Office Web Application - Administrative Interface

Vue 3 web application for warehouse management administrative tasks on desktop/web browsers.

## Overview

The Office Web Application provides comprehensive administrative interfaces for:
- Real-time warehouse dashboard
- Order management and monitoring
- Inventory oversight and analysis
- User and permission management
- Reporting and analytics
- System configuration

Optimized for desktop browsers with mouse and keyboard interaction.

## Features

- **Real-time Dashboard**: Live metrics and KPIs
- **Order Management**: Create, modify, and track orders
- **Inventory Control**: Stock levels, movements, and adjustments
- **Advanced Reporting**: Custom reports and data export
- **User Management**: Role-based access control
- **Multi-tab Interface**: Work with multiple views simultaneously
- **Data Visualization**: Charts and graphs for analytics
- **Bulk Operations**: Batch processing capabilities

## Technology Stack

- **Vue 3**: Progressive JavaScript framework
- **TypeScript**: Type-safe development
- **Vite**: Fast build tool and dev server
- **Pinia**: State management
- **Vue Router**: Client-side routing
- **Tailwind CSS**: Utility-first CSS framework
- **Chart.js / Apache ECharts**: Data visualization
- **Axios**: HTTP client
- **date-fns**: Date manipulation
- **Element Plus / Ant Design Vue**: Component library

## Prerequisites

- Node.js 18+ and npm
- Modern desktop browser (Chrome, Firefox, Safari, Edge)

## Getting Started

### Installation

```bash
cd apps/office-web
npm install
```

### Development

Start the development server:
```bash
npm run dev
```

The app will be available at `http://localhost:3001`

### Building for Production

```bash
npm run build
```

The built files will be in the `dist/` directory.

### Preview Production Build

```bash
npm run preview
```

## Project Structure

```
office-web/
├── public/
│   └── assets/
├── src/
│   ├── assets/            # Images, styles, fonts
│   ├── components/        # Reusable components
│   │   ├── dashboard/
│   │   ├── orders/
│   │   ├── inventory/
│   │   ├── reports/
│   │   ├── users/
│   │   └── common/
│   ├── views/             # Page components
│   │   ├── DashboardView.vue
│   │   ├── OrdersView.vue
│   │   ├── InventoryView.vue
│   │   ├── ReportsView.vue
│   │   └── SettingsView.vue
│   ├── layouts/           # Layout components
│   │   ├── DefaultLayout.vue
│   │   └── AuthLayout.vue
│   ├── stores/            # Pinia stores
│   │   ├── auth.ts
│   │   ├── orders.ts
│   │   ├── inventory.ts
│   │   └── dashboard.ts
│   ├── services/          # API services
│   │   ├── api.ts
│   │   ├── websocket.ts
│   │   └── export.ts
│   ├── router/            # Vue Router
│   ├── composables/       # Composition API composables
│   ├── utils/             # Utility functions
│   ├── types/             # TypeScript types
│   ├── App.vue
│   └── main.ts
├── tests/
│   ├── unit/
│   └── e2e/
├── index.html
├── vite.config.ts
├── tsconfig.json
└── package.json
```

## Key Features

### Dashboard

Real-time warehouse metrics:
- Current inventory levels
- Active orders status
- Picking productivity
- Recent activities
- Performance KPIs
- Alerts and notifications

### Order Management

Comprehensive order handling:
- Create new orders
- Modify existing orders
- Track order status
- Assign pickers
- Batch operations
- Order history

### Inventory Management

Stock control and analysis:
- Real-time stock levels
- Stock movements history
- Low stock alerts
- Reorder suggestions
- Bulk adjustments
- Location management

### Reporting

Custom reports and analytics:
- Predefined report templates
- Custom report builder
- Data export (CSV, Excel, PDF)
- Scheduled reports
- Visual charts and graphs
- Historical trends

### User Management

Access control and administration:
- User accounts
- Role-based permissions
- Activity logs
- Password policies
- Two-factor authentication

## Testing

### Unit Tests

```bash
npm run test:unit
```

### E2E Tests

```bash
npm run test:e2e
```

### Lint

```bash
npm run lint
```

## Configuration

### Environment Variables

Create `.env.local` file:

```env
VITE_API_URL=http://localhost:8080
VITE_WS_URL=ws://localhost:8080/ws
VITE_REPORT_SERVICE_URL=http://localhost:8091
```

### Build Configuration

Edit `vite.config.ts` for build optimization:

```typescript
export default defineConfig({
  build: {
    target: 'es2020',
    minify: 'terser',
    rollupOptions: {
      output: {
        manualChunks: {
          'vendor': ['vue', 'vue-router', 'pinia'],
          'charts': ['chart.js'],
          'ui': ['element-plus']
        }
      }
    }
  }
})
```

## User Interface

### Layout

- **Top Navigation**: Global navigation and user menu
- **Sidebar**: Section navigation and quick actions
- **Main Content**: Primary workspace
- **Footer**: Status and information

### Responsive Design

While optimized for desktop, the app is responsive:
- Desktop: Full features
- Tablet: Simplified layout
- Mobile: Limited view (consider using Tablet PWA)

### Keyboard Shortcuts

- `Ctrl/Cmd + K`: Quick search
- `Ctrl/Cmd + N`: New order
- `Ctrl/Cmd + S`: Save current form
- `Ctrl/Cmd + P`: Print current view
- `Esc`: Close modal/cancel

## State Management

### Pinia Stores

```typescript
// Example: orders store
export const useOrdersStore = defineStore('orders', {
  state: () => ({
    orders: [],
    currentOrder: null,
    filters: {}
  }),
  actions: {
    async fetchOrders() { ... },
    async createOrder(order) { ... },
    async updateOrder(id, data) { ... }
  },
  getters: {
    activeOrders: (state) => state.orders.filter(o => o.status === 'active')
  }
})
```

## API Integration

### REST API

```typescript
// services/api.ts
const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})
```

### WebSocket

Real-time updates via WebSocket:

```typescript
// services/websocket.ts
const ws = new WebSocket(import.meta.env.VITE_WS_URL)
ws.onmessage = (event) => {
  // Handle real-time updates
}
```

## Deployment

### Static Hosting

Deploy to any static hosting:
- Netlify
- Vercel
- GitHub Pages
- AWS S3 + CloudFront
- Azure Static Web Apps

### Docker Deployment

```bash
docker build -t office-web .
docker run -p 3001:80 office-web
```

### Nginx Configuration

```nginx
server {
    listen 80;
    server_name office.warehouse.local;
    
    location / {
        root /usr/share/nginx/html;
        try_files $uri $uri/ /index.html;
    }
    
    location /api {
        proxy_pass http://api-gateway:8080;
    }
}
```

## Performance

- **Code Splitting**: Automatic route-based splitting
- **Lazy Loading**: Components loaded on demand
- **Tree Shaking**: Unused code eliminated
- **Compression**: Gzip/Brotli enabled
- **Caching**: Aggressive browser caching
- **CDN**: Static assets on CDN
- **Bundle Analysis**: `npm run build -- --report`

## Security

- **Authentication**: JWT tokens
- **Authorization**: Role-based access
- **HTTPS**: Required in production
- **CSP**: Content Security Policy
- **CORS**: Proper CORS configuration
- **Input Validation**: Client and server-side
- **XSS Protection**: Sanitized outputs
- **CSRF Protection**: Token-based protection

## Accessibility

- **WCAG 2.1 AA**: Compliance target
- **Screen Readers**: Full support
- **Keyboard Navigation**: Complete keyboard control
- **Focus Management**: Clear focus indicators
- **ARIA Labels**: Proper semantic HTML
- **Color Contrast**: Sufficient contrast ratios

## Browser Support

- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

## Monitoring

- **Error Tracking**: Sentry integration
- **Analytics**: Google Analytics / Mixpanel
- **Performance**: Web Vitals monitoring
- **User Sessions**: Session recording

## Contributing

1. Follow Vue 3 Composition API
2. Use TypeScript for all new code
3. Write unit tests for business logic
4. Document complex components
5. Follow ESLint configuration

## Resources

- [Vue 3 Documentation](https://vuejs.org/)
- [Vite Documentation](https://vitejs.dev/)
- [TypeScript Documentation](https://www.typescriptlang.org/)
- [Tailwind CSS](https://tailwindcss.com/)
- [Pinia Documentation](https://pinia.vuejs.org/)
