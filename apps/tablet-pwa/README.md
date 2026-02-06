# Tablet PWA - Warehouse Floor Operations

Progressive Web App built with Vue 3 for warehouse floor operations on tablets.

## Overview

The Tablet PWA provides intuitive interfaces for warehouse workers to:
- Pick items for orders
- Receive and put away inventory
- Perform cycle counts
- Sign off on completed tasks
- View real-time inventory information

Designed for touch-first interaction with offline-first capabilities.

## Features

- **Offline Support**: Works without constant internet connectivity
- **Touch Optimized**: Large buttons and gesture-friendly interfaces
- **Barcode Scanning**: Camera-based barcode and QR code scanning
- **Digital Signatures**: Capture signatures for pick confirmations
- **Real-time Updates**: Live inventory and order status
- **Progressive Enhancement**: Installable on devices as a native app

## Technology Stack

- **Vue 3**: Progressive JavaScript framework
- **TypeScript**: Type-safe development
- **Vite**: Fast build tool and dev server
- **Pinia**: State management
- **Vue Router**: Client-side routing
- **Vite PWA Plugin**: PWA functionality
- **Tailwind CSS**: Utility-first CSS framework
- **Axios**: HTTP client
- **date-fns**: Date manipulation

## Prerequisites

- Node.js 18+ and npm
- Modern browser with PWA support
- Camera access for barcode scanning (optional)

## Getting Started

### Installation

```bash
cd apps/tablet-pwa
npm install
```

### Development

Start the development server:
```bash
npm run dev
```

The app will be available at `http://localhost:3000`

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
tablet-pwa/
├── public/
│   ├── manifest.json      # PWA manifest
│   └── icons/             # App icons
├── src/
│   ├── assets/            # Images, styles
│   ├── components/        # Reusable components
│   │   ├── picking/
│   │   ├── receiving/
│   │   ├── counting/
│   │   └── common/
│   ├── views/             # Page components
│   │   ├── PickingView.vue
│   │   ├── ReceivingView.vue
│   │   ├── CountingView.vue
│   │   └── SignOffView.vue
│   ├── stores/            # Pinia stores
│   │   ├── auth.ts
│   │   ├── picking.ts
│   │   └── inventory.ts
│   ├── services/          # API services
│   │   ├── api.ts
│   │   └── offline.ts
│   ├── router/            # Vue Router
│   ├── composables/       # Composition API composables
│   ├── utils/             # Utility functions
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

## Key Features Implementation

### Offline Support

The app uses Service Workers and IndexedDB for offline functionality:

```typescript
// Service worker caches API responses and assets
// IndexedDB stores pending operations
// Background sync uploads when connection returns
```

### Barcode Scanning

Camera-based barcode scanning using device camera:

```typescript
// Uses browser's MediaDevices API
// Supports multiple barcode formats
// Falls back to manual entry
```

### Touch Interface

Optimized for touch interaction:
- Large, touch-friendly buttons (min 44x44px)
- Gesture support (swipe, pinch)
- No hover states required
- Haptic feedback on actions

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
VITE_ENABLE_OFFLINE=true
```

### PWA Configuration

Edit `vite.config.ts`:

```typescript
VitePWA({
  registerType: 'autoUpdate',
  manifest: {
    name: 'Warehouse Tablet',
    short_name: 'WH Tablet',
    theme_color: '#4F46E5',
    icons: [...]
  }
})
```

## User Flows

### Picking Workflow
1. Login with credentials
2. View assigned pick lists
3. Scan/select pick list
4. Navigate to location
5. Scan item barcode
6. Confirm quantity
7. Complete pick list
8. Digital signature

### Receiving Workflow
1. Scan delivery note
2. Scan items as received
3. Enter quantities
4. Assign storage locations
5. Confirm receipt
6. Print labels

### Cycle Counting
1. View assigned count tasks
2. Navigate to location
3. Scan items
4. Enter counted quantities
5. Submit count
6. Resolve discrepancies

## Deployment

### As PWA

Deploy to any web server. Users can install from browser:
1. Visit URL in browser
2. Tap "Install" or "Add to Home Screen"
3. App installs like native app

### Docker Deployment

```bash
docker build -t tablet-pwa .
docker run -p 3000:80 tablet-pwa
```

## Browser Support

- Chrome/Edge 90+
- Safari 14+ (iOS)
- Firefox 88+

## Performance Optimization

- Code splitting by route
- Lazy loading components
- Image optimization
- Service worker caching
- Virtual scrolling for large lists
- Debounced search inputs

## Accessibility

- WCAG 2.1 Level AA compliance
- Screen reader support
- Keyboard navigation
- High contrast mode
- Focus indicators
- ARIA labels

## Security

- HTTPS required in production
- Token-based authentication
- Secure storage (encrypted IndexedDB)
- CSP headers
- Input validation
- XSS protection

## Contributing

1. Follow Vue 3 style guide
2. Use TypeScript for type safety
3. Write unit tests for components
4. Test on actual tablet devices
5. Follow commit conventions

## Resources

- [Vue 3 Documentation](https://vuejs.org/)
- [Vite Documentation](https://vitejs.dev/)
- [PWA Documentation](https://web.dev/progressive-web-apps/)
- [Touch Design Guidelines](https://developer.apple.com/design/human-interface-guidelines/ios/user-interaction/gestures/)
