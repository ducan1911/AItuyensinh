FROM python:3.10-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    libopus-dev \
    ffmpeg \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy requirements file
COPY requirements.txt .

# Install Python requirements
RUN pip install --no-cache-dir -r requirements.txt

# Copy server code
COPY server.py .

# Expose port (Render automatically maps PORT)
EXPOSE 8080

# Command to run
CMD ["python", "server.py"]
