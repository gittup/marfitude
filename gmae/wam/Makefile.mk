project: $(wildcard $(d)/*.wam)
$(d)/%.wam: $(src_gmae)/wam.c
	@$(call RM,$@)
