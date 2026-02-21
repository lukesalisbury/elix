
void elix_font_render(elix_font * font, elix_font * emoji, uint32_t character, float &x, float &y, uint32_t next_character) {
	if ( !font || !font->info.numGlyphs) {
		return;
	}
	int index = stbtt_FindGlyphIndex(&font->info, character);

	if ( index ) {
		float scale = stbtt_ScaleForPixelHeight(&font->info, 12);
		float baseline = (font->base_ascent * scale);
		int advance_width, left_sidebearing,width, height, xoff, yoff;

		uint8_t * bitmap = stbtt_GetGlyphBitmap(&font->info, 0, scale, index, &width, &height, &xoff, &yoff);

		//int vert_count = stbtt_GetGlyphShape(&font->info, index, &font->vertices);

		stbtt_GetGlyphHMetrics(&font->info, character, &advance_width, &left_sidebearing);

		for ( int32_t j = 0; j < height; ++j) {
			for ( int32_t i = 0; i < width; ++i) {
				float alpha = (float)bitmap[(j*width)+i] / 255.0;
				//rbgabuffer__pixel(ctx, 0xFF000000, x+i, y+j+yoff+baseline, alpha);
			}
		}

		x += (float)advance_width * scale;
		if ( next_character ) {
			x += scale * stbtt_GetCodepointKernAdvance(&font->info, character, next_character);
		}
		delete bitmap;
	} else if ( emoji ) {
		elix_font_render( emoji, nullptr, character, x, y, next_character);
	}
}

function_results elix_rendertree2item_to_sdlrenderer(elix_rendertree2_item * item, SDL_Renderer * renderer ) {
    ASSERT(item);

	SDL_FRect rect = {(float)item->render_layout.x, (float)item->render_layout.y, (float)item->render_layout.width, (float)item->render_layout.height};

	switch (item->render_layout.display) {
		case ERT_BLOCK:
			SDL_SetRenderDrawColor(renderer, item->render_style.backgroundColour.r, item->render_style.backgroundColour.g, item->render_style.backgroundColour.b, 200); 
			SDL_RenderFillRect(renderer, &rect);

			SDL_SetRenderDrawColor(renderer, item->render_style.colour.r, item->render_style.colour.g, item->render_style.colour.b, item->render_style.colour.a); 			
			SDL_RenderLine(renderer, item->render_layout.x, item->render_layout.y, item->render_layout.x + item->render_layout.width, item->render_layout.y);
			SDL_RenderLine(renderer, item->render_layout.x + item->render_layout.width, item->render_layout.y, item->render_layout.x + item->render_layout.width, item->render_layout.y + item->render_layout.height);
			SDL_RenderLine(renderer, item->render_layout.x + item->render_layout.width, item->render_layout.y + item->render_layout.height, item->render_layout.x, item->render_layout.y + item->render_layout.height);
			SDL_RenderLine(renderer, item->render_layout.x, item->render_layout.y + item->render_layout.height, item->render_layout.x, item->render_layout.y);
			
		break;
		case ERT_INLINE:
		break;
		case ERT_INLINEBLOCK:
		break;
		default:
		break;
	}

	switch (item->data.data_type) {
		case ERTD_STRING:
			if ( item->data.data ) {
				SDL_SetRenderDrawColor(renderer, item->render_style.colour.r, item->render_style.colour.g, item->render_style.colour.b, item->render_style.colour.a); 			
				SDL_RenderDebugText(renderer, item->render_layout.x, item->render_layout.y, (const char*)item->data.data);

				//elix_font_render(&font, &emoji, item->data.data[0], item->render_style.x, item->render_style.y, item->data.data[1]);
			}
			break;

		case ERTD_EMPTY:
			break;
		case ERTD_IMAGE:
			break;
		case ERTD_EXTERNAL:
			break;
		case ERTD_COUNT:
		case ERTD_UPDATE:
		break;



		default:

			SDL_FRect fill_rect = {(float)item->render_layout.x, (float)item->render_layout.y, (float)item->render_layout.width, (float)item->render_layout.height};
			SDL_Rect bordr_rect = {item->render_layout.x, item->render_layout.y, item->render_layout.width, item->render_layout.height};
			bordr_rect.w += 2;
			bordr_rect.h += 2;
			bordr_rect.x -= 1;
			bordr_rect.y -= 1;
			SDL_SetRenderDrawColor(renderer, item->render_style.colour.r, item->render_style.colour.g, item->render_style.colour.b, item->render_style.colour.a); 			
            /*
			SDL_RenderLine(renderer, bordr_rect.x, bordr_rect.y, bordr_rect.x + bordr_rect.w, bordr_rect.y);
            SDL_RenderLine(renderer, bordr_rect.x + bordr_rect.w, bordr_rect.y, bordr_rect.x + bordr_rect.w, bordr_rect.y + bordr_rect.h);
            SDL_RenderLine(renderer, bordr_rect.x + bordr_rect.w, bordr_rect.y + bordr_rect.h, bordr_rect.x, bordr_rect.y + bordr_rect.h);
            SDL_RenderLine(renderer, bordr_rect.x, bordr_rect.y + bordr_rect.h, bordr_rect.x, bordr_rect.y);
			*/


            SDL_SetRenderDrawColor(renderer, item->render_style.backgroundColour.r, item->render_style.backgroundColour.g, item->render_style.backgroundColour.b, 200); 
			SDL_RenderFillRect(renderer, &rect);
			
			break;
	}

	return 0;
}

function_results elix_rendertree2_to_sdlrenderer(elix_rendertree2 * tree, SDL_Renderer * renderer, uint8_t redraw_all) {
	ASSERT(tree);
	ASSERT(renderer);
	if ( tree->item_count ) {
		for (uint16_t item = 0; item < tree->item_count; item++) {
			if ( tree->items[item].render_state != ERTS_HIDDEN ) {
				elix_rendertree2item_to_sdlrenderer(&tree->items[item], renderer);
			}
			
		}
	} else {
		LOG_ERROR("No Render Tree to render");
	}

	return 0;
}



inline elix_dimension min_value(elix_dimension a, elix_dimension b) {
	return a < b ? a : b;
}

uint32_t elix_ui_to_rendertree2(elix_ui * ui, elix_rendertree2 * tree) {
    ASSERT(ui);
    ASSERT(tree);

	//Build the tree
	LOG_INFO("Building Tree");

	for (uint16_t index_count = 0; index_count < 256 && ui->widgets[index_count].type; index_count++) {
		elix_rendertree2_item * item = &tree->items[index_count];
		LOG_INFO("- %d %x %x", index_count, &ui->widgets[index_count], ui->widgets[index_count].parent );

		item->clip_x1 = ui->dimensions.x;
		item->clip_y1 = ui->dimensions.y;
		item->clip_x2 = ui->dimensions.x + ui->dimensions.width;
		item->clip_y2 = ui->dimensions.y + ui->dimensions.height;

		ui->widgets[index_count].render_item = item;

		if ( index_count == 0 ) {
			item->parent = nullptr;  
		}
		if ( ui->widgets[index_count].parent ) {
			item->parent = ui->widgets[index_count].parent->render_item;
			item->clip_x1 += ui->widgets[index_count].parent->position.x;
			item->clip_y1 += ui->widgets[index_count].parent->position.y;
			item->clip_x2 += ui->widgets[index_count].parent->position.x;
			item->clip_y2 += ui->widgets[index_count].parent->position.y; 
			
			item->clip_x2 = min_value(item->clip_x2, ui->widgets[index_count].parent->position.width);
			item->clip_y2 = min_value(item->clip_y2, ui->widgets[index_count].parent->position.height);
		}

		tree->item_count++;
	}


	//Style the tree
	LOG_INFO("Styling Tree");
	for (elix_widget &widget : ui->widgets) {
		
		if ( widget.render_item ) {
			elix_sv32_2 offset = {0,0};
			elix_uv32_2 dimensions = {0,0};
			if ( widget.parent ) {
				offset.x = widget.parent->position.x;
				offset.y = widget.parent->position.y;
				dimensions.width = widget.parent->dimensions.width;
				dimensions.height = widget.parent->dimensions.height;
			}

			widget.render_item->render_style = ui->style_default[widget.type][ELIX_UI_STATE_NORMAL];
			widget.render_item->render_layout.x = widget.position.x + offset.x;
			widget.render_item->render_layout.y = widget.position.y + offset.y;
			widget.render_item->render_layout.width = widget.dimensions.width;
			widget.render_item->render_layout.height = widget.dimensions.height;

			widget.render_item->render_state = ERTS_NORMAL;
			if ( widget.render_item->clip_x1 >= widget.position.x || widget.position.x >= widget.render_item->clip_x2) {
				widget.render_item->render_state = ERTS_CLIPPED;
			}
			if ( widget.render_item->clip_y1 >= widget.position.y || widget.position.y >= widget.render_item->clip_y2) {
				//widget.render_item->render_state = (widget.render_item->render_state == ERTS_CLIPPED) ? ERTS_HIDDEN : ERTS_CLIPPED;	
			}

			elix_widget_add_collision(ui, &widget, offset, dimensions);

			switch (widget.type) {
				
				case ELIX_UI_EMPTY:
					break;
				case ELIX_UI_WINDOW:
					break;
				case ELIX_UI_TEXT:
					if ( widget.resource > 0 && widget.resource < 256 ) {
						widget.render_item->data.data_type = ERTD_STRING;
						widget.render_item->data.index = widget.resource - 1;
						widget.render_item->data.data = ui->text[widget.render_item->data.index];
					}
					break;
				case ELIX_UI_INPUTTEXT:
					if ( widget.resource > 0 && widget.resource < 256 ) {
						widget.render_item->data.data_type = ERTD_STRING;
						widget.render_item->data.index = widget.resource - 1;
						widget.render_item->data.data = ui->text[widget.render_item->data.index];
					}
					break;

				default:
				
					break;

			}
		}
	}

    return 0;
}


function_results elix_ui_handle_sdl_event(elix_ui * ui, SDL_Event * event) {
	ASSERT(ui);
	ASSERT(event);

	switch (event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;

		case SDL_EVENT_DROP_FILE:
			
			break;
		
		case SDL_EVENT_KEY_DOWN:
			if (event->key.key == SDLK_ESCAPE) {
				return SDL_APP_SUCCESS;
			}
			break;
		
		case SDL_EVENT_KEY_UP: 
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			ui->input_global.mouse_button |= SDL_BUTTON_MASK(event->button.button);
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			ui->input_global.mouse_button &= ~SDL_BUTTON_MASK(event->button.button);
			break;

		case SDL_EVENT_MOUSE_MOTION:
			//TODO: Confirm if this is the main mouse for the current window
			ui->input_global.mouse_pointer[0] = (int16_t) event->motion.x;
			ui->input_global.mouse_pointer[1] = (int16_t) event->motion.y;
			break;
	}

	return RESULTS_SUCCESS;
}