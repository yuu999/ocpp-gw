# ECHONET Lite Simulator Dockerfile

FROM node:18-alpine

# Set working directory
WORKDIR /app

# Copy package files
COPY package*.json ./

# Install dependencies
RUN npm ci --only=production

# Copy source code
COPY . .

# Build the application
RUN npm run build

# Create logs directory
RUN mkdir -p logs

# Expose ports
EXPOSE 3610/udp
EXPOSE 8080

# Set environment variables
ENV NODE_ENV=production

# Start the application
CMD ["npm", "start"] 