FROM node:20-alpine
WORKDIR /app
COPY package.json ./
RUN npm install --omit=dev
COPY src ./src
ENV SFU_PORT=4444
EXPOSE 4444
CMD ["npm", "start"]
