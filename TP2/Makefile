CC = gcc
CFLAGS = -Wall -Wextra

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
EXECUTABLES = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%,$(SOURCES))

QUEUE_NAME = mail-queue
LSPAWN_NAME = mail-lspawn
GROUP_NAME = concordia-grupo-criar

MAIL_SEND_NAME = concordia-mail
MAIL_FUL_NAME = concordia-fulfillment

GROUPS_DIR = /var/concordia/groups

FIFO_PATH = /var/concordia/mailfifo
FIFO_OWNER = concordia

all: $(EXECUTABLES) $(BINDIR)/$(QUEUE_NAME) $(BINDIR)/$(LSPAWN_NAME) $(BINDIR)/$(ADD_REM_NAME) $(BINDIR)/$(GROUP_NAME)

$(BINDIR)/%: $(OBJDIR)/%.o | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/$(QUEUE_NAME): $(SRCDIR)/$(QUEUE_NAME).c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@
	sudo chown concordia:concordia $(BINDIR)/$(QUEUE_NAME)
	sudo chmod u+s $(BINDIR)/$(QUEUE_NAME)

$(BINDIR)/$(LSPAWN_NAME): $(SRCDIR)/$(LSPAWN_NAME).c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@
	sudo chown root:root $(BINDIR)/$(LSPAWN_NAME)
	sudo chmod u+s $(BINDIR)/$(LSPAWN_NAME)


$(BINDIR)/$(GROUP_NAME): $(SRCDIR)/$(GROUP_NAME).c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@
	sudo chmod u+x $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

permissions:
	sudo chmod +x $(BINDIR)

create:
	sudo mkdir -p $(GROUPS_DIR)
	sudo chown root:root $(GROUPS_DIR)
	sudo chmod 777 $(GROUPS_DIR)
	sudo mkfifo $(FIFO_PATH)
	sudo chown $(FIFO_OWNER) $(FIFO_PATH)
	sudo chmod 200 $(FIFO_PATH)

install:
	sudo systemctl daemon-reload
	sudo systemctl start $(MAIL_SEND_NAME)
	sudo systemctl enable $(MAIL_SEND_NAME)
	sudo systemctl start $(MAIL_FUL_NAME)
	sudo systemctl enable $(MAIL_FUL_NAME)

uninstall:
	sudo systemctl stop $(MAIL_SEND_NAME)
	sudo systemctl disable $(MAIL_SEND_NAME)
	sudo systemctl stop $(MAIL_FUL_NAME)
	sudo systemctl disable $(MAIL_FUL_NAME)
	sudo systemctl daemon-reload

clean:
	rm -rf $(OBJDIR) $(BINDIR)
